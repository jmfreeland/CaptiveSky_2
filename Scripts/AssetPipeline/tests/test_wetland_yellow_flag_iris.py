"""Source contract for the collisionless yellow-flag iris candidate."""
import ast
from pathlib import Path
import unittest


ASSET = Path(__file__).resolve().parents[1] / "assets" / "wetland_yellow_flag_iris.py"


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
    materials = {
        node.targets[0].slice.value
        for node in ast.walk(tree)
        if isinstance(node, ast.Assign)
        and isinstance(node.targets[0], ast.Subscript)
        and isinstance(node.targets[0].value, ast.Attribute)
        and node.targets[0].value.attr == "PLAIN"
        and isinstance(node.targets[0].slice, ast.Constant)
    }
    collisions = [node for node in calls if isinstance(node.func, ast.Attribute)
                  and node.func.attr.startswith("collide_")]
    return values, materials, collisions, tree


class WetlandYellowFlagIrisTests(unittest.TestCase):
    def test_wetland_species_has_repeatable_distinct_structure(self):
        values, *_ = source_contract()
        self.assertEqual(values["NAME"], "WetlandYellowFlagIris")
        self.assertIn("Iris pseudacorus-inspired", values["PROVENANCE"])
        self.assertEqual(values["LEAF_COUNT"], 10)
        self.assertEqual(values["FLOWER_STALKS"], 3)
        self.assertEqual(values["FLOWERS_PER_STALK"], (1, 2, 1))
        self.assertEqual(len(values["LEAF_LENGTHS"]), values["LEAF_COUNT"])
        self.assertEqual(len(values["LEAF_WIDTHS"]), values["LEAF_COUNT"])
        self.assertGreater(max(values["LEAF_LENGTHS"]), 0.7)
        self.assertGreater(min(values["LEAF_WIDTHS"]), 0.03)

    def test_distinct_leaf_midrib_flower_and_signal_materials_are_used_without_collision(self):
        values, materials, collisions, tree = source_contract()
        expected = {
            "yellow_flag_leaf", "yellow_flag_mid_rib", "yellow_flag_stem",
            "yellow_flag_petal", "yellow_flag_signal",
        }
        self.assertTrue(expected.issubset(materials))
        self.assertEqual(collisions, [])
        imported = {alias.name for node in tree.body if isinstance(node, ast.Import)
                    for alias in node.names}
        self.assertNotIn("random", imported)
        self.assertNotIn("secrets", imported)

    def test_builder_exports_sword_leaves_and_iris_blooms(self):
        *_rest, tree = source_contract()
        functions = {node.name: node for node in tree.body if isinstance(node, ast.FunctionDef)}
        self.assertTrue({"_blade", "_ellipsoid", "build"}.issubset(functions))
        builder = functions["build"]
        names = {node.id for node in ast.walk(builder) if isinstance(node, ast.Name)}
        self.assertTrue({"_blade", "_ellipsoid"}.issubset(names))
        finish_calls = [node for node in ast.walk(builder)
                        if isinstance(node, ast.Call) and isinstance(node.func, ast.Attribute)
                        and node.func.attr == "_finish"]
        self.assertEqual(len(finish_calls), 3)
        triangulation_calls = [node for node in ast.walk(builder)
                               if isinstance(node, ast.Call) and isinstance(node.func, ast.Attribute)
                               and node.func.attr == "triangulate"]
        self.assertTrue(triangulation_calls)
        literals = {node.value for node in ast.walk(builder)
                    if isinstance(node, ast.Constant) and isinstance(node.value, str)}
        self.assertTrue({"yellow_flag_leaf", "yellow_flag_petal", "yellow_flag_signal"}.issubset(literals))

    def test_builder_uses_fanned_leaf_groups_and_surface_set_marks(self):
        *_rest, tree = source_contract()
        functions = {node.name: node for node in tree.body if isinstance(node, ast.FunctionDef)}
        builder = ast.unparse(functions["build"])
        self.assertIn("fan_index = index // 3", builder)
        self.assertIn("leaf_index = index % 3", builder)
        self.assertIn("fall_normal = fall_axis.cross(flower_side).normalized()", builder)
        self.assertIn("fall_normal * 0.003", builder)


if __name__ == "__main__":
    unittest.main()
