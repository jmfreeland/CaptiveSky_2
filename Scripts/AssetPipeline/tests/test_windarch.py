"""Source-level gameplay clearance checks for the scripted Wind Arch collision."""

import ast
from pathlib import Path
import unittest


ASSET_SCRIPT = Path(__file__).resolve().parents[1] / "assets" / "windarch.py"


def collision_boxes():
    tree = ast.parse(ASSET_SCRIPT.read_text(encoding="utf-8"), filename=str(ASSET_SCRIPT))
    boxes = []
    for node in ast.walk(tree):
        if not isinstance(node, ast.Call) or not isinstance(node.func, ast.Attribute):
            continue
        if node.func.attr != "collide_box":
            continue
        size = ast.literal_eval(node.args[0])
        location = ast.literal_eval(node.args[1]) if len(node.args) > 1 else (0, 0, 0)
        boxes.append((size, location))
    return boxes


class WindArchClearanceTests(unittest.TestCase):
    def test_centerline_remains_open_at_character_height(self):
        boxes = collision_boxes()
        self.assertTrue(boxes, "the arch needs explicit collision")
        x, y, z = 0.0, 0.0, 1.8
        for (sx, sy, sz), (cx, cy, cz) in boxes:
            overlaps = (
                abs(x - cx) < sx / 2
                and abs(y - cy) < sy / 2
                and abs(z - cz) < sz / 2
            )
            self.assertFalse(overlaps, "collision blocks the 1.8 m center passage")

    def test_walk_through_has_three_metres_width_and_clear_height(self):
        boxes = collision_boxes()
        z = 1.8
        side_intervals = []
        for (sx, sy, sz), (cx, cy, cz) in boxes:
            if abs(cy) < sy / 2 and abs(z - cz) < sz / 2:
                side_intervals.append((cx - sx / 2, cx + sx / 2))

        left_edge = max(hi for lo, hi in side_intervals if hi < 0)
        right_edge = min(lo for lo, hi in side_intervals if lo > 0)
        self.assertGreaterEqual(right_edge - left_edge, 3.0)

        overhead = [cz - sz / 2 for (sx, sy, sz), (cx, cy, cz) in boxes
                    if cx - sx / 2 < 0 < cx + sx / 2 and abs(cy) < sy / 2]
        self.assertTrue(overhead, "the lintel needs explicit overhead collision")
        self.assertGreaterEqual(min(overhead), 3.5)


if __name__ == "__main__":
    unittest.main()
