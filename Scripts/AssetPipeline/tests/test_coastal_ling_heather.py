"""Source-level contract for the collisionless coastal ling-heather candidate."""
import ast
from pathlib import Path
import unittest


ASSET = Path(__file__).resolve().parents[1] / "assets" / "coastal_ling_heather.py"


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


class CoastalLingHeatherTests(unittest.TestCase):
    def test_low_clump_has_six_distinct_branching_stems(self):
        values, *_ = source_contract()
        self.assertEqual(values["NAME"], "CoastalLingHeather")
        self.assertIn("Calluna vulgaris-inspired", values["PROVENANCE"])
        self.assertEqual(len(values["STEM_BASES"]), 6)
        self.assertEqual(len(values["STEM_HEIGHTS"]), len(values["STEM_BASES"]))
        self.assertEqual(len(set(values["STEM_BASES"])), len(values["STEM_BASES"]))
        self.assertGreater(min(values["STEM_HEIGHTS"]), 0.25)
        self.assertLess(max(values["STEM_HEIGHTS"]), 0.42)
        self.assertEqual(values["BRANCHES_PER_STEM"], 3)
        self.assertEqual(values["RANDOM_SEED"], 20261017)

    def test_leaves_bells_and_materials_are_bounded_for_repeated_foliage(self):
        values, materials, _collisions, tree = source_contract()
        self.assertEqual(values["LEAF_NODES_PER_BRANCH"], 6)
        self.assertEqual(values["LEAVES_PER_NODE"], 4)
        self.assertEqual(values["FLOWERS_PER_BRANCH"], 4)
        self.assertTrue({"heather_stem", "heather_leaf", "heather_bloom"}.issubset(materials))
        bell_rings = next(node for node in ast.walk(tree) if isinstance(node, ast.Assign)
                          and any(isinstance(target, ast.Name) and target.id == "rings"
                                  for target in node.targets)
                          and isinstance(node.value, ast.Tuple))
        self.assertEqual(len(bell_rings.value.elts), 3)

    def test_candidate_is_collisionless_and_uses_seeded_randomness(self):
        values, _materials, collision_calls, tree = source_contract()
        self.assertEqual(collision_calls, [])
        seeded_random = next(node for node in ast.walk(tree) if isinstance(node, ast.Call)
                             and isinstance(node.func, ast.Attribute)
                             and node.func.attr == "Random")
        self.assertIsInstance(seeded_random.args[0], ast.Name)
        self.assertEqual(seeded_random.args[0].id, "RANDOM_SEED")
        self.assertEqual(values["RANDOM_SEED"], 20261017)


if __name__ == "__main__":
    unittest.main()
