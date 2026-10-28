"""Structural gates for the local-only Rat microhabitat composition candidate."""

import ast
import math
from pathlib import Path
import unittest


ASSET = Path(__file__).resolve().parents[1] / "assets" / "rathollow_microhabitat_v1.py"


def constants():
    tree = ast.parse(ASSET.read_text(encoding="utf-8"), filename=str(ASSET))
    result = {}
    for node in ast.walk(tree):
        if not isinstance(node, ast.Assign):
            continue
        for target in node.targets:
            if isinstance(target, ast.Name):
                try:
                    result[target.id] = ast.literal_eval(node.value)
                except (ValueError, TypeError):
                    pass
    return result, tree


class RatHollowMicrohabitatV1Tests(unittest.TestCase):
    def test_rat_entrance_anchors_are_preserved(self):
        config, _ = constants()
        self.assertEqual(config["ASSET_ANCHORS_CM"]["entrance_threshold"], (-78.0, -1.0, 3.0))
        self.assertEqual(config["ASSET_ANCHORS_CM"]["entrance_center"], (-78.0, -1.0, 24.0))

    def test_sparse_fern_reach_leaves_a_clear_approach_to_the_mouth(self):
        config, _ = constants()
        self.assertGreaterEqual(len(config["FERN_PLACEMENTS"]), 3)
        entrance = (-0.78, -0.01)
        for x, y, _z, scale, _yaw in config["FERN_PLACEMENTS"]:
            distance = math.hypot(x - entrance[0], y - entrance[1])
            clearance = distance - config["MAX_FERN_REACH_M"] * scale
            self.assertGreaterEqual(
                clearance,
                config["MIN_ENTRANCE_CLEARANCE_M"],
                f"fern at {(x, y)} crowds the entrance by only {clearance:.2f} m",
            )
            self.assertGreater(scale, 0.0)
            self.assertLessEqual(scale, 0.75)

    def test_visual_ground_cover_is_sparse_and_keeps_collision_source_separate(self):
        config, tree = constants()
        functions = {
            node.name: node for node in ast.walk(tree)
            if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef))
        }
        build_source = ast.get_source_segment(ASSET.read_text(encoding="utf-8"), functions["build"])
        self.assertIn("_HOLLOW.build(A)", build_source)
        self.assertIn("_add_fern(A", build_source)
        self.assertNotIn("A.collide_box", build_source)
        self.assertNotIn("MOSS_SHOULDERS", config)

    def test_asset_uses_existing_reproducible_hollow_and_fern_sources(self):
        source = ASSET.read_text(encoding="utf-8")
        self.assertIn('"rat_hollow_rootarch_v2.py"', source)
        self.assertIn('"woodland_fern.py"', source)
        self.assertIn('NAME = "RatHollowMicrohabitatV1"', source)


if __name__ == "__main__":
    unittest.main()
