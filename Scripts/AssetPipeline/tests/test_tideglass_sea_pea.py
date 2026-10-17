"""Source contract for the low, collisionless Tideglass sea-pea candidate."""
import ast
from pathlib import Path
import unittest


ASSET = Path(__file__).resolve().parents[1] / "assets" / "tideglass_sea_pea.py"


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
    material_keys = {
        node.targets[0].slice.value
        for node in ast.walk(tree)
        if isinstance(node, ast.Assign)
        and isinstance(node.targets[0], ast.Subscript)
        and isinstance(node.targets[0].value, ast.Attribute)
        and node.targets[0].value.attr == "PLAIN"
        and isinstance(node.targets[0].slice, ast.Constant)
    }
    collision_calls = [node for node in calls if isinstance(node.func, ast.Attribute)
                       and node.func.attr.startswith("collide_")]
    return values, material_keys, collision_calls, tree


class TideglassSeaPeaTests(unittest.TestCase):
    def test_low_spreading_native_coast_candidate_has_repeatable_structure(self):
        values, *_ = source_contract()
        self.assertEqual(values["NAME"], "TideglassSeaPea")
        self.assertIn("Lathyrus japonicus-inspired", values["PROVENANCE"])
        self.assertEqual(values["RUNNER_COUNT"], 6)
        self.assertEqual(values["LEAF_PAIRS_PER_RUNNER"], 3)
        self.assertEqual(values["LEAFLET_PAIRS_PER_LEAF"], 3)
        self.assertEqual(values["FLOWERS_PER_RUNNER"], 2)

    def test_leaf_and_bloom_materials_are_separate_and_no_randomness_or_collision_is_used(self):
        _values, materials, collisions, tree = source_contract()
        self.assertTrue({"sea_pea_stem", "sea_pea_leaf", "sea_pea_flower"}.issubset(materials))
        self.assertEqual(collisions, [])
        imported = {alias.name for node in tree.body if isinstance(node, ast.Import)
                    for alias in node.names}
        self.assertNotIn("random", imported)
        self.assertNotIn("secrets", imported)

    def test_builder_uses_flowering_leaflet_geometry_and_exports_collisionless_meshes(self):
        _values, _materials, _collisions, tree = source_contract()
        functions = {node.name: node for node in tree.body if isinstance(node, ast.FunctionDef)}
        self.assertTrue({"_leaflet", "_pea_bloom", "build"}.issubset(functions))
        names = {node.id for node in ast.walk(functions["build"])
                 if isinstance(node, ast.Name)}
        self.assertIn("_leaflet", names)
        self.assertIn("_pea_bloom", names)
        finish_calls = [node for node in ast.walk(functions["build"])
                        if isinstance(node, ast.Call) and isinstance(node.func, ast.Attribute)
                        and node.func.attr == "_finish"]
        builder_literals = {node.value for node in ast.walk(functions["build"])
                            if isinstance(node, ast.Constant) and isinstance(node.value, str)}
        self.assertTrue(finish_calls)
        self.assertTrue({"sea_pea_leaf", "sea_pea_flower"}.issubset(builder_literals))


if __name__ == "__main__":
    unittest.main()
