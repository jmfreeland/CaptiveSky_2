"""Source-level contract for the low woodland wild-strawberry asset."""
import ast
from pathlib import Path
import unittest


ASSET = Path(__file__).resolve().parents[1] / "assets" / "woodland_wildstrawberry.py"


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
        node.args[0].value for node in calls
        if isinstance(node.func, ast.Attribute) and node.func.attr == "rock"
        and node.args and isinstance(node.args[0], ast.Constant)
    }
    collision_calls = [node for node in calls if isinstance(node.func, ast.Attribute)
                       and node.func.attr.startswith("collide_")]
    return values, material_keys, collision_calls


class WoodlandWildStrawberryTests(unittest.TestCase):
    def test_patch_has_a_bounded_repeatable_species_composition(self):
        values, _materials, _collision = source_contract()
        self.assertEqual(values["NAME"], "WoodlandWildStrawberry")
        self.assertEqual(values["LEAF_COUNTS"], (4, 4, 4, 3))
        self.assertEqual(sum(values["LEAF_COUNTS"]), 15)
        self.assertEqual(len(values["FLOWER_POSITIONS"]), 5)
        self.assertEqual(len(values["BERRY_POSITIONS"]), 4)

    def test_palette_covers_leaf_stem_flower_pollen_and_fruit(self):
        tree = ast.parse(ASSET.read_text(encoding="utf-8"), filename=str(ASSET))
        palette = {
            node.targets[0].slice.value
            for node in ast.walk(tree)
            if isinstance(node, ast.Assign)
            and isinstance(node.targets[0], ast.Subscript)
            and isinstance(node.targets[0].value, ast.Attribute)
            and node.targets[0].value.attr == "PLAIN"
            and isinstance(node.targets[0].slice, ast.Constant)
        }
        self.assertTrue({"strawberry_leaf", "strawberry_stem", "strawberry_flower",
                         "strawberry_pollen", "strawberry_fruit"}.issubset(palette))

    def test_groundcover_does_not_add_character_collision(self):
        _values, _materials, collision_calls = source_contract()
        self.assertEqual(collision_calls, [])


if __name__ == "__main__":
    unittest.main()
