"""Source-level contract for the tall woodland-edge foxglove candidate."""
import ast
from pathlib import Path
import unittest


ASSET = Path(__file__).resolve().parents[1] / "assets" / "woodland_foxglove.py"


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
    palette_keys = {
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
    function_names = {node.name for node in tree.body if isinstance(node, ast.FunctionDef)}
    return values, palette_keys, collision_calls, function_names


class WoodlandFoxgloveTests(unittest.TestCase):
    def test_candidate_is_three_deterministic_spikes_for_mid_distance_structure(self):
        values, _palette, _collision, _functions = source_contract()
        self.assertEqual(values["NAME"], "WoodlandFoxglove")
        self.assertEqual(len(values["SPIKE_HEIGHTS"]), 3)
        self.assertEqual(values["SPIKE_HEIGHTS"], (1.28, 1.14, 1.04))
        self.assertEqual(len(values["PLANT_BASES"]), 3)
        self.assertEqual(values["FLOWERS_PER_SPIKE"], 8)
        self.assertEqual(values["ROSETTE_LEAVES_PER_PLANT"], 5)

    def test_palette_separates_leaves_stems_bells_and_throat_marks(self):
        _values, palette, _collision, _functions = source_contract()
        self.assertTrue({"foxglove_leaf", "foxglove_stem", "foxglove_bloom",
                         "foxglove_throat"}.issubset(palette))

    def test_bells_have_a_distinct_throat_detail_mesh(self):
        _values, _palette, _collision, functions = source_contract()
        self.assertIn("_bell", functions)
        self.assertIn("_throat_marks", functions)

    def test_groundcover_candidate_does_not_add_character_collision(self):
        _values, _palette, collisions, _functions = source_contract()
        self.assertEqual(collisions, [])


if __name__ == "__main__":
    unittest.main()
