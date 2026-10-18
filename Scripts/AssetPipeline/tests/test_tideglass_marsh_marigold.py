"""Source contract for the collisionless Tideglass marsh-marigold candidate."""
import ast
from pathlib import Path
import unittest


ASSET = Path(__file__).resolve().parents[1] / "assets" / "tideglass_marsh_marigold.py"


def source_contract():
    tree = ast.parse(ASSET.read_text(encoding="utf-8"), filename=str(ASSET))
    values = {}
    material_keys = set()
    calls = [node for node in ast.walk(tree) if isinstance(node, ast.Call)]
    for node in tree.body:
        if isinstance(node, ast.Assign):
            for target in node.targets:
                if isinstance(target, ast.Name):
                    try:
                        values[target.id] = ast.literal_eval(node.value)
                    except (ValueError, TypeError):
                        pass
    for node in ast.walk(tree):
        if (isinstance(node, ast.Assign) and isinstance(node.targets[0], ast.Subscript)
                and isinstance(node.targets[0].value, ast.Attribute)
                and node.targets[0].value.attr == "PLAIN"
                and isinstance(node.targets[0].slice, ast.Constant)):
            material_keys.add(node.targets[0].slice.value)
    collision_calls = [node for node in calls if isinstance(node.func, ast.Attribute)
                       and node.func.attr.startswith("collide_")]
    imported = {alias.name for node in tree.body if isinstance(node, ast.Import)
                for alias in node.names}
    return values, material_keys, collision_calls, imported, tree


class TideglassMarshMarigoldTests(unittest.TestCase):
    def test_candidate_has_native_wet_margin_provenance_and_distinct_form(self):
        values, *_ = source_contract()
        self.assertEqual(values["NAME"], "TideglassMarshMarigold")
        self.assertIn("Caltha palustris-inspired", values["PROVENANCE"])
        self.assertEqual(len(values["ROSETTE_CENTRES_M"]), 4)
        self.assertEqual(values["LEAVES_PER_ROSETTE"], 3)
        self.assertEqual(len(values["FLOWER_BASES_M"]), 5)
        self.assertEqual(values["FLOWER_HEIGHTS_M"], (0.30, 0.36, 0.33, 0.28, 0.32))
        self.assertEqual(values["SEPALS_PER_FLOWER"], 8)
        self.assertGreater(values["LEAF_WIDTH_M"], values["SEPAL_WIDTH_M"] * 3)
        self.assertEqual(values["LEAF_LONGITUDINAL_SEGMENTS"], 12)
        self.assertEqual(values["LEAF_LATERAL_SEGMENTS"], 6)
        self.assertEqual(len(values["LEAF_HALF_WIDTH_PROFILE"]), 13)
        self.assertEqual(values["LEAF_HALF_WIDTH_PROFILE"][0], 0.0)
        self.assertGreater(values["LEAF_HALF_WIDTH_PROFILE"][5], 1.0)
        self.assertEqual(values["LEAF_HALF_WIDTH_PROFILE"][-1], 0.0)
        self.assertEqual(values["SEPAL_LONGITUDINAL_SEGMENTS"], 8)
        self.assertEqual(values["SEPAL_LATERAL_SEGMENTS"], 4)
        self.assertEqual(len(values["SEPAL_HALF_WIDTH_PROFILE"]), 9)
        self.assertEqual(values["SEPAL_HALF_WIDTH_PROFILE"][0], 0.0)
        self.assertEqual(values["SEPAL_HALF_WIDTH_PROFILE"][-1], 0.0)
        self.assertEqual(max(values["SEPAL_HALF_WIDTH_PROFILE"]), 1.0)
        self.assertGreater(values["SEPAL_TIP_RISE_M"], 0.01)
        self.assertGreater(values["SEPAL_EDGE_CUP_M"], 0.004)

    def test_geometry_is_deterministic_collisionless_and_grouped_by_material(self):
        _values, materials, collisions, imported, _tree = source_contract()
        self.assertEqual(collisions, [])
        self.assertNotIn("random", imported)
        self.assertTrue({"marigold_leaf", "marigold_stem", "marigold_sepal",
                         "marigold_centre"}.issubset(materials))

    def test_zero_width_endpoints_are_single_vertex_fans_with_fbx_smoothing(self):
        values, *_rest, tree = source_contract()
        functions = {node.name: node for node in tree.body if isinstance(node, ast.FunctionDef)}
        self.assertEqual(values["FBX_MESH_SMOOTH_TYPE"], "FACE")
        leaf = ast.unparse(functions["_leaf"])
        sepal = ast.unparse(functions["_sepal"])
        self.assertIn("row in (0, LEAF_LONGITUDINAL_SEGMENTS)", leaf)
        self.assertIn("row in (0, SEPAL_LONGITUDINAL_SEGMENTS)", sepal)
        for geometry in (leaf, sepal):
            self.assertIn("rows[0][0]", geometry)
            self.assertIn("rows[-1][0]", geometry)

    def test_builder_uses_shared_meshes_for_organic_parts(self):
        *_rest, tree = source_contract()
        functions = {node.name: node for node in tree.body if isinstance(node, ast.FunctionDef)}
        self.assertTrue({"_tube", "_leaf", "_sepal", "build"}.issubset(functions))
        builder = ast.unparse(functions["build"])
        self.assertEqual(builder.count("A._finish("), 4)
        self.assertIn("_tube(stems", builder)
        self.assertIn("_leaf(leaves", builder)
        self.assertIn("_sepal(flower_sepal_mesh", builder)
        self.assertIn("triangulate", ast.unparse(functions["build"]))


if __name__ == "__main__":
    unittest.main()
