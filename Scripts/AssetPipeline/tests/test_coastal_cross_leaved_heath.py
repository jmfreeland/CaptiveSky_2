"""Source-level checks for the collisionless cross-leaved heath candidate."""
import ast
from pathlib import Path
import unittest


ASSET = Path(__file__).resolve().parents[1] / "assets" / "coastal_cross_leaved_heath.py"


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
    collision_calls = [node for node in ast.walk(tree) if isinstance(node, ast.Call)
                       and isinstance(node.func, ast.Attribute)
                       and node.func.attr.startswith("collide_")]
    return values, collision_calls, tree


class CoastalCrossLeavedHeathTests(unittest.TestCase):
    def test_species_and_wet_heath_provenance_are_explicit(self):
        values, *_ = source_contract()
        self.assertEqual(values["NAME"], "CoastalCrossLeavedHeath")
        self.assertIn("Erica tetralix-inspired", values["PROVENANCE"])
        self.assertEqual(values["RANDOM_SEED"], 20261018)

    def test_four_leaf_whorls_and_clustered_pendant_bells_are_bounded(self):
        values, *_ = source_contract()
        self.assertEqual(values["LEAVES_PER_WHORL"], 4)
        self.assertEqual(len(values["LEADER_BASES"]), 7)
        self.assertEqual(len(values["LEADER_HEIGHTS"]), len(values["LEADER_BASES"]))
        self.assertEqual(values["SIDE_BRANCHES_PER_LEADER"], 2)
        self.assertEqual(values["LEADER_WHORLS"], 6)
        self.assertEqual(values["SIDE_WHORLS_PER_BRANCH"], 2)
        self.assertEqual(values["BELLS_PER_CLUSTER"], 3)
        self.assertEqual(values["BELL_SEGMENTS"], 5)
        self.assertGreater(min(values["LEADER_HEIGHTS"]), 0.30)
        self.assertLess(max(values["LEADER_HEIGHTS"]), 0.40)

    def test_candidate_does_not_add_collision_and_has_a_single_seeded_rng(self):
        values, collisions, tree = source_contract()
        self.assertEqual(collisions, [])
        random_calls = [node for node in ast.walk(tree) if isinstance(node, ast.Call)
                        and isinstance(node.func, ast.Attribute)
                        and node.func.attr == "Random"]
        self.assertEqual(len(random_calls), 1)
        self.assertIsInstance(random_calls[0].args[0], ast.Name)
        self.assertEqual(random_calls[0].args[0].id, "RANDOM_SEED")
        self.assertEqual(values["RANDOM_SEED"], 20261018)


if __name__ == "__main__":
    unittest.main()
