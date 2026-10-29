"""Regression checks for the separate weathered RatHollow V4 candidate."""

import ast
from pathlib import Path
import unittest


ASSET_SCRIPT = Path(__file__).resolve().parents[1] / "assets" / "rat_hollow_rootarch_v4.py"


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


class RatHollowRootArchV4Tests(unittest.TestCase):
    def test_candidate_uses_a_new_destination_without_mutating_prior_versions(self):
        config = constants()
        self.assertEqual(config["NAME"], "RatHollowRootArchBurrowWeatheredV4")
        source = ASSET_SCRIPT.read_text(encoding="utf-8")
        self.assertIn('"rat_hollow_rootarch_v3.py"', source)
        self.assertIn("_BASE.build(A)", source)

    def test_bark_tile_and_asset_scale_match_the_validated_base(self):
        config = constants()
        self.assertEqual(config["BARK_TEXTURE_TILE_METRES"], 1.2)
        self.assertEqual(len(config["ASSET_ANCHORS_CM"]["entrance_threshold"]), 3)

    def test_inner_wood_is_lighter_but_remains_natural_and_non_emissive(self):
        color = constants()["HOLLOWWOOD_BASE_COLOR"]
        self.assertGreater(color[0], 0.40)
        self.assertGreater(color[1], 0.25)
        self.assertLess(color[0], 0.55)
        self.assertLess(color[2], color[0])

    def test_moss_is_distributed_as_bounded_crown_patches(self):
        patches = constants()["MOSS_CROWN_PATCHES"]
        self.assertGreaterEqual(len(patches), 5)
        self.assertLessEqual(len(patches), 8)
        self.assertEqual([patch[0] for patch in patches], sorted(patch[0] for patch in patches))
        self.assertTrue(all(-1.0 <= patch[1] <= 1.0 and 0.08 <= patch[2] <= 0.23 for patch in patches))
        source = ASSET_SCRIPT.read_text(encoding="utf-8")
        self.assertIn("crown_z + 0.004", source)
        self.assertIn('A.rock(\n            "moss"', source)


if __name__ == "__main__":
    unittest.main()
