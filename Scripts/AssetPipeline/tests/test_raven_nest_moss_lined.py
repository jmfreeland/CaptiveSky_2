"""Source contracts for the reversible moss-lined Raven nest variation."""
import ast
from pathlib import Path
import unittest


PIPE = Path(__file__).resolve().parents[1]
ASSET = PIPE / "assets" / "raven_nest_moss_lined.py"
BASE_ASSET = PIPE / "assets" / "raven_nest.py"


class RavenNestMossLinedTests(unittest.TestCase):
    def test_variant_reuses_the_validated_collisionless_weave(self):
        tree = ast.parse(ASSET.read_text(encoding="utf-8"), filename=str(ASSET))
        base_tree = ast.parse(BASE_ASSET.read_text(encoding="utf-8"), filename=str(BASE_ASSET))
        self.assertEqual(
            ast.literal_eval(next(node.value for node in tree.body
                                  if isinstance(node, ast.Assign)
                                  and any(isinstance(target, ast.Name) and target.id == "NAME"
                                          for target in node.targets))),
            "RavenNest_WovenBowl_MossLined",
        )
        self.assertTrue(any(
            isinstance(node, ast.Call) and isinstance(node.func, ast.Attribute)
            and isinstance(node.func.value, ast.Name) and node.func.value.id == "raven_nest"
            and node.func.attr == "build"
            for node in ast.walk(tree)
        ))
        self.assertFalse(any(
            isinstance(node, ast.Call) and isinstance(node.func, ast.Attribute)
            and isinstance(node.func.value, ast.Name) and node.func.value.id == "A"
            and node.func.attr.startswith("collide_")
            for node in ast.walk(base_tree)
        ), "the reused weave must remain collisionless")

    def test_moss_is_sparse_and_uses_a_muted_rough_material(self):
        source = ASSET.read_text(encoding="utf-8")
        tree = ast.parse(source, filename=str(ASSET))
        self.assertIn('"BaseColor": [0.145, 0.18, 0.09]', source)
        self.assertIn('"Roughness": 0.98', source)
        self.assertIn('for index in range(18):', source)
        self.assertIn('raven_nest._twig(A, points, 0.00125', source)
        self.assertNotIn('A.rock(', source)
        self.assertNotIn("A.collide_", source)

    def test_script_has_valid_python_syntax(self):
        ast.parse(ASSET.read_text(encoding="utf-8"), filename=str(ASSET))


if __name__ == "__main__":
    unittest.main()
