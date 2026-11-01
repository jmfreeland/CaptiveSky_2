"""Source-level checks for the bark-scale-only RatHollow V10 variant."""

import ast
from pathlib import Path
import unittest


ASSET_DIR = Path(__file__).resolve().parents[1] / "assets"
BUILDER = ASSET_DIR / "rathollow_hidden_v10.py"


class RatHollowHiddenV10Tests(unittest.TestCase):
    def test_v10_is_a_single_variable_scale_change_from_v9(self):
        tree = ast.parse(BUILDER.read_text(encoding="utf-8"))
        constants = {
            target.id: ast.literal_eval(node.value)
            for node in tree.body
            if isinstance(node, ast.Assign)
            for target in node.targets
            if isinstance(target, ast.Name) and target.id in {"NAME", "BARK_TILE_METRES"}
        }
        self.assertEqual(constants["NAME"], "RatHollow_HiddenV10")
        self.assertEqual(constants["BARK_TILE_METRES"], 1.8)
        source = BUILDER.read_text(encoding="utf-8")
        self.assertIn('"rathollow_hidden_v9.py"', source)
        self.assertIn("rathollow_hidden_v9.BARK_TILE_METRES = BARK_TILE_METRES", source)
        self.assertIn("rathollow_hidden_v9.build(A)", source)

    def test_v9_passes_bark_tile_through_without_changing_older_callers(self):
        source = (ASSET_DIR / "rathollow.py").read_text(encoding="utf-8")
        self.assertIn("def _hollow_log(A, conform_moss=False, bark_tile=1.2):", source)
        self.assertIn('"box", "X", bark_tile, True, 55)', source)
        v10 = BUILDER.read_text(encoding="utf-8")
        self.assertIn('ASSET_ANCHORS_CM = dict(rathollow_hidden_v9.ASSET_ANCHORS_CM)', v10)


if __name__ == "__main__":
    unittest.main()
