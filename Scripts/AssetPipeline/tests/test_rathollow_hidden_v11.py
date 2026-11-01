"""Source-level checks for the finer bark-scale RatHollow V11 variant."""

import ast
from pathlib import Path
import unittest


ASSET_DIR = Path(__file__).resolve().parents[1] / "assets"
BUILDER = ASSET_DIR / "rathollow_hidden_v11.py"


class RatHollowHiddenV11Tests(unittest.TestCase):
    def test_v11_is_a_single_variable_scale_change_from_v9(self):
        tree = ast.parse(BUILDER.read_text(encoding="utf-8"))
        constants = {
            target.id: ast.literal_eval(node.value)
            for node in tree.body
            if isinstance(node, ast.Assign)
            for target in node.targets
            if isinstance(target, ast.Name) and target.id in {"NAME", "BARK_TILE_METRES"}
        }
        self.assertEqual(constants["NAME"], "RatHollow_HiddenV11")
        self.assertEqual(constants["BARK_TILE_METRES"], 0.8)
        source = BUILDER.read_text(encoding="utf-8")
        self.assertIn('"rathollow_hidden_v9.py"', source)
        self.assertIn("rathollow_hidden_v9.BARK_TILE_METRES = BARK_TILE_METRES", source)
        self.assertIn("rathollow_hidden_v9.build(A)", source)


if __name__ == "__main__":
    unittest.main()
