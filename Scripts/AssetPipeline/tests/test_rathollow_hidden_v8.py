"""Source-level contract checks for the unplaced hidden RatHollow V8 study."""

from pathlib import Path
import unittest


ASSET_DIR = Path(__file__).resolve().parents[1] / "assets"
BUILDER = ASSET_DIR / "rathollow_hidden_v8.py"


class RatHollowHiddenV8Tests(unittest.TestCase):
    def test_v8_preserves_v7_anchors_and_adds_open_passage_root_shoulders(self):
        source = BUILDER.read_text(encoding="utf-8")
        self.assertIn('NAME = "RatHollow_HiddenV8"', source)
        self.assertIn('"entrance": (-132.0, 0.0, 66.0)', (ASSET_DIR / "rathollow_hidden_v7.py").read_text(encoding="utf-8"))
        self.assertIn('"nest_bed": (-32.0, 0.0, 32.0)', (ASSET_DIR / "rathollow_hidden_v7.py").read_text(encoding="utf-8"))
        self.assertIn("rathollow_hidden_v7.build(A)", source)
        self.assertIn('name="entranceRootShoulderNear"', source)
        self.assertIn('name="entranceRootShoulderFar"', source)
        self.assertNotIn("A.collide_box", source)
        self.assertIn("_triangulate_export_meshes(A)", source)


if __name__ == "__main__":
    unittest.main()
