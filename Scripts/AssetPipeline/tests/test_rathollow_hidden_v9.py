"""Source-level checks for the unplaced hidden RatHollow V9 art iteration."""

import ast
from pathlib import Path
import unittest


ASSET_DIR = Path(__file__).resolve().parents[1] / "assets"
BUILDER = ASSET_DIR / "rathollow_hidden_v9.py"


def read_points(tree, name):
    for node in tree.body:
        if isinstance(node, ast.Assign) and any(
            isinstance(target, ast.Name) and target.id == name for target in node.targets
        ):
            return ast.literal_eval(node.value)
    raise AssertionError(f"Missing literal geometry constant: {name}")


class RatHollowHiddenV9Tests(unittest.TestCase):
    def test_v9_preserves_the_v7_home_anchors_and_shell(self):
        source = BUILDER.read_text(encoding="utf-8")
        baseline = (ASSET_DIR / "rathollow_hidden_v7.py").read_text(encoding="utf-8")
        self.assertIn('NAME = "RatHollow_HiddenV9"', source)
        self.assertIn('"entrance": (-132.0, 0.0, 66.0)', source)
        self.assertIn('"nest_bed": (-32.0, 0.0, 32.0)', source)
        self.assertIn('rathollow._hollow_log(A, conform_moss=True)', source)
        self.assertIn('rathollow._triangulate_export_meshes(A)', source)
        self.assertIn("ASSET_ANCHORS_CM", baseline)
        self.assertEqual((ASSET_DIR / "rathollow.py").read_text(encoding="utf-8").count("A.collide_box("), 5)

    def test_entrance_roots_stay_inside_the_front_plane_and_clear_the_passage(self):
        tree = ast.parse(BUILDER.read_text(encoding="utf-8"))
        for name in ("CANOPY_POINTS_M", "NEAR_SHOULDER_POINTS_M", "FAR_SHOULDER_POINTS_M"):
            points = read_points(tree, name)
            self.assertTrue(all(point[0] >= -1.30 for point in points), name)
        near = read_points(tree, "NEAR_SHOULDER_POINTS_M")
        far = read_points(tree, "FAR_SHOULDER_POINTS_M")
        self.assertTrue(all(point[1] <= -0.39 for point in near))
        self.assertTrue(all(point[1] >= 0.41 for point in far))
        self.assertIn('BASE_ENTRANCE_TIPS_TO_REMOVE = ("entranceSplinter_", "splitRimEast_", "splitRimWest_")',
                      BUILDER.read_text(encoding="utf-8"))

    def test_ground_roots_remain_inside_the_trunk_length(self):
        tree = ast.parse(BUILDER.read_text(encoding="utf-8"))
        for name in ("NEAR_GROUND_ROOT_POINTS_M", "FAR_GROUND_ROOT_POINTS_M"):
            points = read_points(tree, name)
            self.assertTrue(all(-1.30 <= point[0] <= 1.24 for point in points), name)


if __name__ == "__main__":
    unittest.main()
