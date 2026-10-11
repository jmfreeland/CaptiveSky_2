"""Ground-level Rat capsule clearance for the scripted RatHollow collision."""

import ast
from pathlib import Path
import unittest


ASSET_SCRIPT = Path(__file__).resolve().parents[1] / "assets" / "rathollow.py"


def collision_boxes():
    tree = ast.parse(ASSET_SCRIPT.read_text(encoding="utf-8"), filename=str(ASSET_SCRIPT))
    boxes = []
    for node in ast.walk(tree):
        if not isinstance(node, ast.Call) or not isinstance(node.func, ast.Attribute):
            continue
        if node.func.attr != "collide_box":
            continue
        size = ast.literal_eval(node.args[0])
        location_arg = next((arg.value for arg in node.keywords if arg.arg == "loc"), None)
        location = ast.literal_eval(location_arg) if location_arg is not None else (0, 0, 0)
        boxes.append((size, location))
    return boxes


class RatHollowClearanceTests(unittest.TestCase):
    def test_grounded_rat_capsule_has_clear_route_through_tunnel(self):
        """Approximate the authored 16 x 24 cm capsule swept along the X tunnel."""
        boxes = collision_boxes()
        self.assertTrue(boxes, "the hollow needs explicit simple collision")
        capsule_radius = 0.08
        capsule_height = 0.24
        # Check the entrance and the full interior rather than just a single point.
        for x in [round(-1.0 + i * 0.1, 2) for i in range(15)]:
            capsule_min = (x - capsule_radius, -capsule_radius, 0.0)
            capsule_max = (x + capsule_radius, capsule_radius, capsule_height)
            for (sx, sy, sz), (cx, cy, cz) in boxes:
                box_min = (cx - sx / 2, cy - sy / 2, cz - sz / 2)
                box_max = (cx + sx / 2, cy + sy / 2, cz + sz / 2)
                overlaps = all(capsule_min[i] < box_max[i] and capsule_max[i] > box_min[i]
                               for i in range(3))
                self.assertFalse(
                    overlaps,
                    f"capsule at x={x:.2f} intersects collision box {((sx, sy, sz), (cx, cy, cz))}",
                )

    def test_ground_level_passage_has_room_for_capsule_with_margin(self):
        """Collision below the tunnel ceiling must leave a generous central gap."""
        boxes = collision_boxes()
        intervals = []
        for (sx, sy, sz), (cx, cy, cz) in boxes:
            if cx - sx / 2 < 0 < cx + sx / 2 and cz - sz / 2 < 0.24 and cz + sz / 2 > 0:
                intervals.append((cy - sy / 2, cy + sy / 2))

        left_edge = max(hi for lo, hi in intervals if hi < 0)
        right_edge = min(lo for lo, hi in intervals if lo > 0)
        self.assertGreaterEqual(right_edge - left_edge, 0.40)
        runners = [(size, location) for size, location in boxes if size[2] <= 0.12]
        self.assertEqual(len(runners), 2, "the floor should have only two narrow side runners")
        self.assertTrue(all(abs(location[1]) >= 0.29 for _, location in runners))


if __name__ == "__main__":
    unittest.main()
