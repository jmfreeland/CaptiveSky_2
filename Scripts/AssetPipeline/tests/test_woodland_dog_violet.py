"""Source contract for the collisionless spring dog-violet candidate."""
import ast
from pathlib import Path
import unittest


ASSET = Path(__file__).resolve().parents[1] / "assets" / "woodland_dog_violet.py"


def source_contract():
    tree = ast.parse(ASSET.read_text(encoding="utf-8"), filename=str(ASSET))
    values = {}
    calls = [node for node in ast.walk(tree) if isinstance(node, ast.Call)]
    colors = {
        node.targets[0].slice.value: ast.literal_eval(node.value)
        for node in ast.walk(tree)
        if isinstance(node, ast.Assign)
        and isinstance(node.targets[0], ast.Subscript)
        and isinstance(node.targets[0].value, ast.Attribute)
        and node.targets[0].value.attr == "PLAIN"
    }
    for node in tree.body:
        if isinstance(node, ast.Assign):
            for target in node.targets:
                if isinstance(target, ast.Name):
                    try:
                        values[target.id] = ast.literal_eval(node.value)
                    except (ValueError, TypeError):
                        pass
    return values, calls, colors


class WoodlandDogVioletTests(unittest.TestCase):
    def test_low_colony_is_bounded_and_has_a_small_spring_bloom_roster(self):
        values, _calls, _colors = source_contract()
        self.assertEqual(values["NAME"], "WoodlandDogViolet")
        self.assertEqual(values["FLOWER_COUNT"], 5)
        self.assertEqual(len(values["PLANT_BASES"]), 4)
        self.assertLessEqual(values["MODEL_SCALE"], 0.75)
        self.assertLessEqual(values["MAX_HEIGHT_M"], 0.15)
        self.assertIn("Collisionless", values["PROVENANCE"])

    def test_flowers_use_five_purple_petals_and_pale_centres_without_collision(self):
        _values, calls, colors = source_contract()
        self.assertTrue({"violet_leaf_dark", "violet_leaf_light", "violet_petal",
                         "violet_throat"}.issubset(colors))
        self.assertTrue(any(isinstance(call.func, ast.Name) and call.func.id == "_heart_leaf"
                            for call in calls))
        self.assertTrue(any(isinstance(call.func, ast.Name) and call.func.id == "_flower_petal"
                            for call in calls))
        self.assertTrue(any(isinstance(call.func, ast.Name) and call.func.id == "_flower_throat"
                            for call in calls))
        self.assertFalse(any(isinstance(call.func, ast.Attribute)
                             and call.func.attr.startswith("collide_") for call in calls))


if __name__ == "__main__":
    unittest.main()
