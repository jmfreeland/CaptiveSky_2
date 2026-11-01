"""Source-level contract for the collisionless fly-agaric-inspired woodland patch."""
import ast
from pathlib import Path
import unittest


ASSET = Path(__file__).resolve().parents[1] / "assets" / "woodland_fly_agaric_cluster.py"


def source_contract():
    tree = ast.parse(ASSET.read_text(encoding="utf-8"), filename=str(ASSET))
    values = {}
    for node in tree.body:
        if isinstance(node, (ast.Assign, ast.AnnAssign)):
            targets = node.targets if isinstance(node, ast.Assign) else [node.target]
            for target in targets:
                if isinstance(target, ast.Name):
                    try:
                        values[target.id] = ast.literal_eval(node.value)
                    except (ValueError, TypeError):
                        pass
    calls = [node for node in ast.walk(tree) if isinstance(node, ast.Call)]
    collision_calls = [node for node in calls if isinstance(node.func, ast.Attribute)
                       and node.func.attr.startswith("collide_")]
    colors = {
        node.targets[0].slice.value: ast.literal_eval(node.value)
        for node in ast.walk(tree)
        if isinstance(node, ast.Assign)
        and isinstance(node.targets[0], ast.Subscript)
        and isinstance(node.targets[0].value, ast.Attribute)
        and node.targets[0].value.attr == "PLAIN"
    }
    return values, collision_calls, colors, calls


class WoodlandFlyAgaricClusterTests(unittest.TestCase):
    def test_cluster_has_repeatable_varied_forms_and_two_cap_tones(self):
        values, _collisions, colors, _calls = source_contract()
        self.assertEqual(values["NAME"], "WoodlandFlyAgaricCluster")
        self.assertEqual(len(values["MUSHROOMS"]), 5)
        self.assertEqual({entry[-1] for entry in values["MUSHROOMS"]},
                         {"agaric_cap_scarlet", "agaric_cap_russet"})
        self.assertGreater(len({entry[3] for entry in values["MUSHROOMS"]}), 1)
        self.assertGreater(len({entry[2] for entry in values["MUSHROOMS"]}), 1)
        self.assertTrue({"agaric_stem", "agaric_cap_scarlet", "agaric_cap_russet",
                         "agaric_veil"}.issubset(colors))

    def test_patch_uses_bounded_veil_geometry_and_no_collision(self):
        _values, collisions, _colors, calls = source_contract()
        self.assertEqual(collisions, [])
        rock_calls = [node for node in calls if isinstance(node.func, ast.Attribute)
                      and node.func.attr == "rock"]
        self.assertEqual(len(rock_calls), 1)
        self.assertTrue(any(k.arg == "sub" and isinstance(k.value, ast.Constant)
                            and k.value.value == 1 for k in rock_calls[0].keywords))


if __name__ == "__main__":
    unittest.main()
