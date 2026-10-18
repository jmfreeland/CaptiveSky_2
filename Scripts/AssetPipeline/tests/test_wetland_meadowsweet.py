"""Source contracts for the collisionless wet-meadow meadowsweet candidate."""

import ast
from pathlib import Path
import unittest


ASSET = Path(__file__).resolve().parents[1] / "assets" / "wetland_meadowsweet.py"


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
    materials = {
        node.targets[0].slice.value
        for node in ast.walk(tree)
        if isinstance(node, ast.Assign)
        and isinstance(node.targets[0], ast.Subscript)
        and isinstance(node.targets[0].value, ast.Attribute)
        and node.targets[0].value.attr == "PLAIN"
        and isinstance(node.targets[0].slice, ast.Constant)
    }
    calls = [node for node in ast.walk(tree) if isinstance(node, ast.Call)]
    collisions = [node for node in calls if isinstance(node.func, ast.Attribute)
                  and node.func.attr.startswith("collide_")]
    return values, materials, collisions, tree


class WetlandMeadowsweetTests(unittest.TestCase):
    def test_candidate_is_deterministic_and_species_shaped(self):
        values, *_ = source_contract()
        self.assertEqual(values["NAME"], "WetlandMeadowsweet")
        self.assertIn("Filipendula ulmaria-inspired", values["PROVENANCE"])
        self.assertEqual(values["STEM_COUNT"], 5)
        self.assertEqual(values["LEAVES_PER_STEM"], 2)
        self.assertEqual(values["LEAFLET_PAIRS"], 4)
        self.assertEqual(values["FLOWER_BRANCHES_PER_STEM"], 5)
        self.assertEqual(values["FLORETS_PER_BRANCH"], 12)

    def test_material_groups_cover_stems_pinnate_leaves_and_florets(self):
        _values, materials, collisions, tree = source_contract()
        self.assertEqual(materials, {
            "meadowsweet_leaf", "meadowsweet_stem", "meadowsweet_flower",
            "meadowsweet_centre",
        })
        self.assertEqual(collisions, [])
        imports = {alias.name for node in tree.body if isinstance(node, ast.Import)
                   for alias in node.names}
        self.assertNotIn("random", imports)
        self.assertNotIn("secrets", imports)

    def test_builder_constructs_compound_leaves_and_open_flower_sprays(self):
        *_rest, tree = source_contract()
        functions = {node.name: node for node in tree.body if isinstance(node, ast.FunctionDef)}
        self.assertTrue({"_stem_tube", "_leaflet", "_floret", "_floret_centre", "build"}.issubset(functions))
        builder = ast.unparse(functions["build"])
        self.assertIn("LEAFLET_PAIRS", builder)
        self.assertIn("FLOWER_BRANCHES_PER_STEM", builder)
        self.assertIn("FLORETS_PER_BRANCH", builder)
        self.assertEqual(builder.count("A._finish("), 4)
        self.assertIn("bmesh.ops.triangulate", builder)

    def test_source_is_python_syntax_valid(self):
        ast.parse(ASSET.read_text(encoding="utf-8"), filename=str(ASSET))


if __name__ == "__main__":
    unittest.main()
