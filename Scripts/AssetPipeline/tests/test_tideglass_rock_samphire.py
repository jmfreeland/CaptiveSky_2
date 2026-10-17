"""Source contract for the collisionless Tideglass rock-samphire candidate."""
import ast
from pathlib import Path
import unittest


ASSET = Path(__file__).resolve().parents[1] / "assets" / "tideglass_rock_samphire.py"


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
    tube_materials = {
        node.args[0].value for node in calls
        if isinstance(node.func, ast.Attribute) and node.func.attr == "tube"
        and node.args and isinstance(node.args[0], ast.Constant)
    }
    return values, material_keys, collision_calls, tube_materials, tree


class TideglassRockSamphireTests(unittest.TestCase):
    def test_candidate_has_a_compact_stable_clump_and_distinct_umbels(self):
        values, *_ = source_contract()
        self.assertEqual(values["NAME"], "TideglassRockSamphire")
        self.assertIn("Crithmum maritimum-inspired", values["PROVENANCE"])
        self.assertEqual(len(values["STEM_BASES"]), 5)
        self.assertEqual(len(set(values["STEM_BASES"])), 5)
        self.assertEqual(len(values["STEM_HEIGHTS"]), 5)
        self.assertGreater(min(values["STEM_HEIGHTS"]), 0.27)
        self.assertLess(max(values["STEM_HEIGHTS"]), 0.45)
        self.assertEqual(values["BRANCHES_PER_STEM"], 3)
        self.assertEqual(values["LEAF_NODES_PER_BRANCH"], 2)
        self.assertEqual(values["LEAFLETS_PER_NODE"], 3)
        self.assertEqual(values["FLORETS_PER_UMBEL"], 7)

    def test_muted_saltmarsh_palette_and_geometry_are_split_by_material(self):
        _values, materials, _collisions, tube_materials, tree = source_contract()
        self.assertTrue({"samphire_stem", "samphire_leaf", "samphire_flower"}.issubset(materials))
        finish_materials = {
            node.args[2].value for node in ast.walk(tree) if isinstance(node, ast.Call)
            and isinstance(node.func, ast.Attribute) and node.func.attr == "_finish"
            and len(node.args) > 2 and isinstance(node.args[2], ast.Constant)
        }
        self.assertIn("samphire_stem", tube_materials)
        self.assertIn("samphire_leaf", tube_materials)
        self.assertIn("samphire_flower", finish_materials)

    def test_candidate_is_collisionless_and_uses_no_randomness(self):
        _values, _materials, collision_calls, _tubes, tree = source_contract()
        self.assertEqual(collision_calls, [])
        imported = {alias.name for node in tree.body if isinstance(node, ast.Import)
                    for alias in node.names}
        self.assertNotIn("random", imported)


if __name__ == "__main__":
    unittest.main()
