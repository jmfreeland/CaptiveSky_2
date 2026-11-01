"""Source-level contract for the collisionless woodland fern candidate."""
import ast
from pathlib import Path
import unittest


ASSET = Path(__file__).resolve().parents[1] / "assets" / "woodland_fern_cluster.py"


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
    collisions = [node for node in calls if isinstance(node.func, ast.Attribute)
                  and node.func.attr.startswith("collide_")]
    colors = {
        node.targets[0].slice.value: ast.literal_eval(node.value)
        for node in ast.walk(tree)
        if isinstance(node, ast.Assign)
        and isinstance(node.targets[0], ast.Subscript)
        and isinstance(node.targets[0].value, ast.Attribute)
        and node.targets[0].value.attr == "PLAIN"
    }
    return values, collisions, colors, calls


class WoodlandFernClusterTests(unittest.TestCase):
    def test_cluster_has_seven_deterministic_fronds_and_paired_pinnae(self):
        values, _collisions, _colors, _calls = source_contract()
        self.assertEqual(values["NAME"], "WoodlandFernCluster")
        self.assertEqual(values["FROND_COUNT"], 7)
        self.assertGreaterEqual(values["PINNAE_PAIRS"], 5)
        self.assertIn("no generated textures", values["PROVENANCE"])

    def test_rosette_uses_a_green_palette_and_never_adds_collision(self):
        _values, collisions, colors, calls = source_contract()
        self.assertEqual(collisions, [])
        self.assertTrue({"fern_rachis", "fern_deep_green", "fern_leaf_green",
                         "fern_young_green"}.issubset(colors))
        self.assertTrue(any(isinstance(node.func, ast.Name)
                            and node.func.id == "_leaflet" for node in calls))
        self.assertTrue(any(isinstance(node.func, ast.Attribute)
                            and node.func.attr == "tube" for node in calls))


if __name__ == "__main__":
    unittest.main()
