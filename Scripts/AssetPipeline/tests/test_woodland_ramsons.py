"""Source-level contract for the collisionless ramsons understory candidate."""

import ast
from pathlib import Path
import unittest


ASSET = Path(__file__).resolve().parents[1] / "assets" / "woodland_ramsons.py"


def source_contract():
    tree = ast.parse(ASSET.read_text(encoding="utf-8"), filename=str(ASSET))
    values = {}
    for node in tree.body:
        if isinstance(node, ast.Assign):
            for target in node.targets:
                if isinstance(target, ast.Name):
                    try:
                        values[target.id] = ast.literal_eval(node.value)
                    except (ValueError, TypeError):
                        pass
    calls = [node for node in ast.walk(tree) if isinstance(node, ast.Call)]
    collisions = [node for node in calls if isinstance(node.func, ast.Attribute)
                  and node.func.attr.startswith("collide_")]
    return tree, values, calls, collisions


class WoodlandRamsonsTests(unittest.TestCase):
    def test_patch_has_seven_paired_leaf_rosettes_and_three_flowering_scapes(self):
        _tree, values, _calls, _collisions = source_contract()
        self.assertEqual(values["NAME"], "WoodlandRamsons")
        self.assertEqual(len(values["ROSETTES"]), 7)
        self.assertEqual(values["FLOWERING_ROSETTES"], (0, 3, 6))
        self.assertEqual(values["SCAPE_HEIGHTS"], (0.38, 0.43, 0.40))
        self.assertLessEqual(max(point[0] for point in values["ROSETTES"]) -
                             min(point[0] for point in values["ROSETTES"]), 0.5)
        self.assertLessEqual(max(point[1] for point in values["ROSETTES"]) -
                             min(point[1] for point in values["ROSETTES"]), 0.55)
        self.assertEqual(values["FLORETS_PER_UMBEL"], 16)
        self.assertIn("collisionless", values["PROVENANCE"])

    def test_asset_is_deterministic_collisionless_and_uses_distinct_material_groups(self):
        tree, _values, calls, collisions = source_contract()
        self.assertEqual(collisions, [])
        self.assertFalse(any(isinstance(node, ast.Name) and node.id == "random"
                             for node in ast.walk(tree)))
        colors = {
            node.targets[0].slice.value: ast.literal_eval(node.value)
            for node in ast.walk(tree)
            if isinstance(node, ast.Assign)
            and isinstance(node.targets[0], ast.Subscript)
            and isinstance(node.targets[0].value, ast.Attribute)
            and node.targets[0].value.attr == "PLAIN"
        }
        self.assertTrue({"ramson_leaf_deep", "ramson_leaf_fresh", "ramson_stem",
                         "ramson_flower", "ramson_pollen"}.issubset(colors))
        self.assertTrue(any(isinstance(node.func, ast.Name) and node.func.id == "_leaflet"
                            for node in calls))
        self.assertTrue(any(isinstance(node.func, ast.Name) and node.func.id == "_star_floret"
                            for node in calls))
        self.assertTrue(any(isinstance(node.func, ast.Name) and node.func.id == "_pollen_dot"
                            for node in calls))
        self.assertTrue(any(isinstance(node.func, ast.Attribute) and node.func.attr == "tube"
                            for node in calls))


if __name__ == "__main__":
    unittest.main()
