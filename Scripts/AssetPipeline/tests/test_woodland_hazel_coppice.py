"""Source-level contract for the mid-distance woodland hazel candidate."""
import ast
from pathlib import Path
import unittest


ASSET = Path(__file__).resolve().parents[1] / "assets" / "woodland_hazel_coppice.py"


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
    collision_calls = [node for node in calls if isinstance(node.func, ast.Attribute)
                       and node.func.attr.startswith("collide_")]
    return values, collision_calls


class WoodlandHazelCoppiceTests(unittest.TestCase):
    def test_leaf_profile_is_a_smooth_broad_ovate_outline(self):
        values, _collisions = source_contract()
        rows = values["LEAF_ROWS"]
        widths = values["LEAF_WIDTH_PROFILE"]
        self.assertEqual(values["NAME"], "WoodlandHazelCoppice")
        self.assertEqual(len(rows), 7)
        self.assertEqual(len(widths), len(rows))
        self.assertEqual(rows[0], 0.0)
        self.assertEqual(rows[-1], 1.0)
        self.assertTrue(all(a < b for a, b in zip(rows, rows[1:])))
        self.assertLess(widths[0], 0.05)
        self.assertLess(widths[-1], 0.05)
        self.assertEqual(max(range(len(widths)), key=widths.__getitem__), 3)

    def test_coppice_remains_soft_foliage_without_character_collision(self):
        _values, collisions = source_contract()
        self.assertEqual(collisions, [])


if __name__ == "__main__":
    unittest.main()
