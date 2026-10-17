"""Rat-scale entry and collision checks for the reversible RootArch V2 asset."""

import ast
from pathlib import Path
import unittest


ASSET_SCRIPT = Path(__file__).resolve().parents[1] / "assets" / "rat_hollow_rootarch_v2.py"


def constants():
    tree = ast.parse(ASSET_SCRIPT.read_text(encoding="utf-8"), filename=str(ASSET_SCRIPT))
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
    return result


def aabb(size, center):
    return tuple((center[i] - size[i] / 2, center[i] + size[i] / 2) for i in range(3))


class RatHollowRootArchV2Tests(unittest.TestCase):
    def test_torn_entrance_opens_through_the_lower_bark_rim(self):
        config = constants()
        gap = config["ENTRANCE_RIM_GAP_SEGMENTS"]
        self.assertEqual(gap, (12, 13, 14, 15, 16, 17))
        self.assertIn(15, gap, "the opening must break through the lowest rim segment")
        self.assertEqual(list(gap), list(range(min(gap), max(gap) + 1)))
        self.assertEqual(config["RIM_SEGMENT_COUNT"], 20)

    def test_entry_threshold_is_near_ground_and_tunnel_floor_rises_gradually(self):
        config = constants()
        self.assertAlmostEqual(config["ASSET_ANCHORS_CM"]["entrance_threshold"][2], 3.0)
        stations = config["INTERIOR_FLOOR_STATIONS"]
        floor_heights = [station[3] for station in stations]
        self.assertLessEqual(floor_heights[0], 0.04)
        self.assertEqual(floor_heights, sorted(floor_heights))
        self.assertLessEqual(max(b - a for a, b in zip(floor_heights, floor_heights[1:])), 0.14)

    def test_simple_collision_keeps_the_rat_capsule_clear_of_the_tunnel(self):
        config = constants()
        boxes = config["COLLISION_BOXES"]
        self.assertEqual(len(boxes), 8)
        side_walls = [
            (size, location) for size, location in boxes
            if abs(size[1] - 0.06) < 1e-6 and abs(location[1]) > 0.2
        ]
        self.assertEqual(len(side_walls), 2)
        gap_width = min(abs(location[1]) - size[1] / 2 for size, location in side_walls) * 2
        self.assertGreaterEqual(gap_width, 0.40)

        floor_boxes = [
            (size, location) for size, location in boxes
            if abs(size[1] - 0.56) < 1e-6 and abs(size[2] - 0.04) < 1e-6
        ]
        self.assertEqual(len(floor_boxes), 4)
        floor_tops = sorted(location[2] + size[2] / 2 for size, location in floor_boxes)
        for actual, expected in zip(floor_tops, [0.04, 0.08, 0.12, 0.16]):
            self.assertAlmostEqual(actual, expected)
        self.assertTrue(all(b - a <= 0.040001 for a, b in zip(floor_tops, floor_tops[1:])))

        # A 16 x 24 cm capsule rests on each ramp section without intersecting
        # either side wall, the roof, the ramp surface, or the closed back.
        for size, location in floor_boxes:
            floor = aabb(size, location)
            x = (floor[0][0] + floor[0][1]) / 2
            z = floor[2][1]
            capsule = ((x - 0.08, x + 0.08), (-0.08, 0.08), (z, z + 0.24))
            for other_size, other_location in boxes:
                other = aabb(other_size, other_location)
                overlaps = all(capsule[i][0] < other[i][1] and capsule[i][1] > other[i][0]
                               for i in range(3))
                self.assertFalse(
                    overlaps,
                    f"Rat capsule on floor section at x={x:.2f}, z={z:.2f} hits {other_size} at {other_location}",
                )

    def test_nest_bedding_tracks_the_lowered_floor(self):
        config = constants()
        self.assertLess(config["NEST_DETAIL_Z_OFFSET_M"], 0.0)
        self.assertGreater(config["REAR_SOIL_Z_OFFSET_M"], 0.0)


if __name__ == "__main__":
    unittest.main()
