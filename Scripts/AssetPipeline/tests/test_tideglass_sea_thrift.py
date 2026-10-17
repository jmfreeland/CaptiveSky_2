"""Source-level contract for the Tideglass sea-thrift asset candidate."""
import ast
from pathlib import Path
import unittest


ASSET = Path(__file__).resolve().parents[1] / "assets" / "tideglass_sea_thrift.py"


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
    tube_materials = {
        node.args[0].value for node in calls
        if isinstance(node.func, ast.Attribute) and node.func.attr == "tube"
        and node.args and isinstance(node.args[0], ast.Constant)
    }
    collision_calls = [node for node in calls if isinstance(node.func, ast.Attribute)
                       and node.func.attr.startswith("collide_")]
    return values, material_keys, tube_materials, collision_calls, tree


class TideglassSeaThriftTests(unittest.TestCase):
    def test_candidate_has_six_stable_stems_and_round_flower_heads(self):
        values, *_ = source_contract()
        self.assertEqual(values["NAME"], "TideglassSeaThrift")
        self.assertIn("no generated textures", values["PROVENANCE"])
        self.assertEqual(len(values["STEM_BASES"]), 6)
        self.assertEqual(len(values["FLOWER_HEADS"]), 6)
        self.assertEqual(values["ROSETTE_LEAVES_PER_STEM"], 4)
        self.assertEqual(values["FLORETS_PER_HEAD"], 15)

    def test_coastal_palette_and_distinct_five_petal_geometry_are_present(self):
        _values, materials, tube_materials, _collisions, tree = source_contract()
        self.assertTrue({"sea_thrift_leaf", "sea_thrift_stem", "sea_thrift_petal",
                         "sea_thrift_centre"}.issubset(materials))
        self.assertIn("sea_thrift_stem", tube_materials)
        floret = next(node for node in tree.body if isinstance(node, ast.FunctionDef)
                      and node.name == "_floret")
        petal_loops = [node for node in ast.walk(floret) if isinstance(node, ast.For)
                       and isinstance(node.target, ast.Name) and node.target.id == "petal_index"]
        self.assertEqual(len(petal_loops), 1)
        self.assertIsInstance(petal_loops[0].iter, ast.Call)
        self.assertEqual(petal_loops[0].iter.func.id, "range")
        self.assertEqual(ast.literal_eval(petal_loops[0].iter.args[0]), 5)

    def test_candidate_is_deterministic_and_never_adds_collision(self):
        _values, _materials, _tubes, collision_calls, tree = source_contract()
        self.assertEqual(collision_calls, [])
        imported = {alias.name for node in tree.body if isinstance(node, ast.Import)
                    for alias in node.names}
        self.assertNotIn("random", imported)


if __name__ == "__main__":
    unittest.main()
