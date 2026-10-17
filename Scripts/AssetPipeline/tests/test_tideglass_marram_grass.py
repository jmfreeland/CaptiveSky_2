"""Source-level contract for the Tideglass marram-grass candidate."""
import ast
from pathlib import Path
import unittest


ASSET = Path(__file__).resolve().parents[1] / "assets" / "tideglass_marram_grass.py"


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


class TideglassMarramGrassTests(unittest.TestCase):
    def test_candidate_has_deterministic_dune_clump_and_three_seed_heads(self):
        values, *_ = source_contract()
        self.assertEqual(values["NAME"], "TideglassMarramGrass")
        self.assertIn("no generated textures", values["PROVENANCE"])
        self.assertEqual(values["LEAF_COUNT"], 9)
        self.assertEqual(len(values["LEAF_LENGTHS"]), values["LEAF_COUNT"])
        self.assertEqual(len(values["LEAF_WIDTHS"]), values["LEAF_COUNT"])
        self.assertEqual(len(values["SEED_HEADS"]), 3)
        self.assertEqual(values["SPIKELETS_PER_HEAD"], 36)

    def test_palette_and_wind_lean_read_as_a_distinct_coastal_species(self):
        values, materials, _collisions, _tree = source_contract()
        self.assertTrue({"marram_leaf", "marram_stem", "marram_seed"}.issubset(materials))
        self.assertLess(values["WIND_LEAN"][0], 0.0)
        self.assertGreater(abs(values["WIND_LEAN"][1]), 0.0)

    def test_candidate_is_deterministic_and_never_adds_collision(self):
        _values, _materials, collision_calls, tree = source_contract()
        self.assertEqual(collision_calls, [])
        imported = {alias.name for node in tree.body if isinstance(node, ast.Import)
                    for alias in node.names}
        self.assertNotIn("random", imported)


if __name__ == "__main__":
    unittest.main()
