import importlib.util
import json
import tempfile
import unittest
from pathlib import Path


SCRIPT = Path(__file__).resolve().parents[1] / "Build-WorldAtlas.py"
SPEC = importlib.util.spec_from_file_location("build_world_atlas", SCRIPT)
assert SPEC and SPEC.loader
ATLAS = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(ATLAS)


class WorldAtlasInputTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)

    def tearDown(self):
        self.temp.cleanup()

    def write_json(self, name, value):
        path = self.root / name
        path.write_text(json.dumps(value), encoding="utf-8")
        return path

    def test_empty_legacy_snapshot_states_that_no_history_was_recorded(self):
        world_path = self.write_json("world.json", {"version": 1, "curios": [], "nests": []})
        trails_path = self.write_json("trails.json", {"version": 1, "cells": []})
        chronicle_path = self.root / "chronicle.jsonl"
        chronicle_path.write_text("", encoding="utf-8")

        world = ATLAS.load_world_state(world_path)
        trails = ATLAS.load_trails(trails_path)
        chronicle = ATLAS.load_chronicle(chronicle_path)
        page = ATLAS.render({"world": world, "trails": trails, "chronicle": chronicle,
                             "anchors": [], "anchors_skipped": 0})

        self.assertIn("No spatial evidence in the supplied snapshot", page)
        self.assertIn("does not infer activity", page)
        self.assertIn("Snapshot time not supplied", page)

    def test_legacy_trail_cells_keep_negative_region_flooring(self):
        trail_path = self.write_json("trails.json", {
            "version": 1,
            "cells": [[-1, 0, 7, -149, 75, 4, 0.0, 0.0]],
        })

        loaded = ATLAS.load_trails(trail_path)

        self.assertEqual(loaded["cells"][0]["region"], [-1, 0])
        self.assertEqual(loaded["cells"][0]["last_use_day"], -1)

    def test_chronicle_string_region_coordinates_are_mapped_and_unknown_types_skipped(self):
        chronicle_path = self.root / "chronicle.jsonl"
        chronicle_path.write_text("\n".join([
            json.dumps({"type": "trail", "event": "trail_emerged", "region_x": "-1", "region_y": "2",
                        "day": 4, "clock": "12:00", "text": "A route is wearing in."}),
            json.dumps({"type": "future_event", "day": 4, "text": "unknown"}),
            "{broken",
        ]), encoding="utf-8")

        loaded = ATLAS.load_chronicle(chronicle_path)
        event = loaded["events"][0]

        self.assertEqual(event["region"], [-1, 2])
        self.assertEqual(event["x"], -600.0)
        self.assertEqual(event["y"], 3000.0)
        self.assertEqual(loaded["unsupported_types"], 1)
        self.assertEqual(loaded["invalid_lines"], 1)

    def test_spatial_crab_wildlife_event_with_string_at_coordinates_maps_to_its_source(self):
        chronicle_path = self.root / "chronicle.jsonl"
        chronicle_path.write_text(json.dumps({
            "type": "wildlife", "event": "crab_wrack_investigation", "source_id": "wrack_7",
            "wrack_id": "7", "at": "-120.50,900.25,30.00", "day": 4, "clock": "12:00",
            "agent": "TidepoolCrab", "text": "A Tideglass shore crab investigates storm wrack 7.",
        }), encoding="utf-8")

        loaded = ATLAS.load_chronicle(chronicle_path)
        event = loaded["events"][0]
        page = ATLAS.render({"world": {"version": 3, "points": [], "skipped_points": 0},
                             "trails": {"version": 2, "cells": [], "skipped_cells": 0},
                             "chronicle": loaded, "anchors": [], "anchors_skipped": 0})

        self.assertEqual((event["x"], event["y"]), (-120.5, 900.25))
        self.assertEqual(event["event"], "crab_wrack_investigation")
        self.assertEqual(event["source_id"], "wrack_7")
        self.assertIn("A Tideglass shore crab investigates storm wrack 7.", page)
        self.assertIn("source wrack_7", page)

    def test_out_of_bounds_and_mismatched_cells_are_rejected(self):
        for row in (
            [0, 0, 2, 1, ATLAS.MAX_COORD_CM + 1, 0, 0, 0, 0],
            [0, 0, 2, 1, 151, 0, 0, 0, 0],
        ):
            path = self.write_json("bad-trails.json", {"version": 2, "cells": [row]})
            with self.subTest(row=row), self.assertRaises(ATLAS.AtlasInputError):
                ATLAS.load_trails(path)

    def test_large_trail_ledgers_are_bounded(self):
        path = self.write_json("large-trails.json", {"version": 1, "cells": [[]] * (ATLAS.MAX_TRAIL_CELLS + 1)})

        with self.assertRaises(ATLAS.AtlasInputError):
            ATLAS.load_trails(path)

    def test_corrupt_world_state_is_rejected_without_partial_render(self):
        path = self.root / "broken.json"
        path.write_text("{not json", encoding="utf-8")

        with self.assertRaises(ATLAS.AtlasInputError):
            ATLAS.load_world_state(path)

    def test_resident_text_is_escaped_and_event_provenance_is_visible(self):
        world = {"version": 3, "points": [], "skipped_points": 0}
        trails = {"version": 2, "cells": [], "skipped_cells": 0}
        chronicle = {
            "events": [{"type": "decision", "day": 3, "clock": "08:10", "timestamp": "",
                        "agent": "Agent_Aster_01", "text": "<script>not executable</script>",
                        "event": "", "x": -100, "y": 200,
                        "region": list(ATLAS._region_for_xy(-100, 200))}],
            "invalid_lines": 0, "unsupported_types": 0, "last_recorded_at": None,
        }

        page = ATLAS.render({"world": world, "trails": trails, "chronicle": chronicle,
                             "anchors": [], "anchors_skipped": 0}, "copied 2026-11-23")

        self.assertIn("&lt;script&gt;not executable&lt;/script&gt;", page)
        self.assertNotIn("<script>not executable</script>", page)
        self.assertIn('data-day="3"', page)
        self.assertIn("Snapshot label: copied 2026-11-23", page)
        self.assertIn("positive Y is shown north/up", page)


if __name__ == "__main__":
    unittest.main()
