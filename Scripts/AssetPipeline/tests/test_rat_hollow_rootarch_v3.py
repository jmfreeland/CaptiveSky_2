"""Regression checks for the readable, rat-scale RootArch V3 candidate."""

import ast
from pathlib import Path
import unittest


ASSET_SCRIPT = Path(__file__).resolve().parents[1] / "assets" / "rat_hollow_rootarch_v3.py"


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


class RatHollowRootArchV3Tests(unittest.TestCase):
    def test_candidate_is_separate_from_existing_rat_hollows(self):
        config = constants()
        self.assertEqual(config["NAME"], "RatHollowRootArchBurrowLightwellV3")
        self.assertNotEqual(config["NAME"], "RatHollowRootArchBurrow")

    def test_lower_arch_is_wider_and_taller_but_stays_rat_scale(self):
        config = constants()
        first_mouth = config["ENTRANCE_MOUTH_RINGS"][0]
        width_cm = first_mouth[7] * 200
        height_cm = first_mouth[8] * 200
        self.assertAlmostEqual(width_cm, 40.0)
        self.assertAlmostEqual(height_cm, 46.0)
        self.assertGreaterEqual((width_cm - 16.0) / 2, 12.0)
        top_cm = (first_mouth[6] + first_mouth[8]) * 100
        threshold_cm = config["ASSET_ANCHORS_CM"]["entrance_threshold"][2]
        self.assertGreaterEqual(top_cm - threshold_cm - 24.0, 20.0)

    def test_broken_rim_opens_down_to_the_ground_entry(self):
        gap = constants()["ENTRANCE_RIM_GAP_SEGMENTS"]
        self.assertEqual(gap, tuple(range(11, 19)))
        self.assertIn(15, gap, "the bottom rim segment must be removed")

    def test_interior_material_is_warm_and_not_emissive(self):
        color = constants()["HOLLOWWOOD_BASE_COLOR"]
        self.assertEqual(len(color), 3)
        self.assertGreater(color[0], 0.30)
        self.assertGreater(color[1], 0.18)
        self.assertLess(color[0], 0.50, "avoid a glowing or freshly painted entrance")

    def test_reuses_v2_geometry_without_mutating_or_replacing_its_asset(self):
        source = ASSET_SCRIPT.read_text(encoding="utf-8")
        self.assertIn('"rat_hollow_rootarch_v2.py"', source)
        self.assertIn("_BASE.build(A)", source)
        self.assertIn('lib.PLAIN["hollowwood"]', source)


if __name__ == "__main__":
    unittest.main()
