"""Source contracts for the collisionless Common Frog asset candidate."""
import ast
from pathlib import Path
import unittest


PIPE = Path(__file__).resolve().parents[1]
ASSET = PIPE / "assets" / "common_frog.py"


class CommonFrogCandidateTests(unittest.TestCase):
    def test_candidate_has_expected_identity_and_species_proportions(self):
        source = ASSET.read_text(encoding="utf-8")
        tree = ast.parse(source, filename=str(ASSET))
        assignments = {target.id: node.value for node in tree.body
                       if isinstance(node, ast.Assign)
                       for target in node.targets if isinstance(target, ast.Name)}
        self.assertEqual(ast.literal_eval(assignments["NAME"]), "Tideglass_CommonFrog_Candidate")
        self.assertIn("0.034, 0.022, 0.014", source)
        self.assertIn("temporalMask", source)
        self.assertIn("hindThigh", source)

    def test_candidate_is_collisionless_and_uses_only_local_mesh_helpers(self):
        tree = ast.parse(ASSET.read_text(encoding="utf-8"), filename=str(ASSET))
        collisions = [node for node in ast.walk(tree)
                      if isinstance(node, ast.Call) and isinstance(node.func, ast.Attribute)
                      and isinstance(node.func.value, ast.Name) and node.func.value.id == "A"
                      and node.func.attr.startswith("collide_")]
        self.assertEqual(collisions, [])
        self.assertFalse(any(isinstance(node, ast.Call) and isinstance(node.func, ast.Name)
                            and node.func.id in {"open", "urlopen"} for node in ast.walk(tree)))
        self.assertTrue(any(isinstance(node, ast.Call) and isinstance(node.func, ast.Attribute)
                            and isinstance(node.func.value, ast.Name) and node.func.value.id == "A"
                            and node.func.attr == "rock" for node in ast.walk(tree)))
        self.assertTrue(any(isinstance(node, ast.Call) and isinstance(node.func, ast.Attribute)
                            and isinstance(node.func.value, ast.Name) and node.func.value.id == "A"
                            and node.func.attr == "tube" for node in ast.walk(tree)))

    def test_script_has_valid_python_syntax(self):
        ast.parse(ASSET.read_text(encoding="utf-8"), filename=str(ASSET))


if __name__ == "__main__":
    unittest.main()
