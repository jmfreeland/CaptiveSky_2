"""Source-level contract for the Tideglass soft-rush asset candidate."""
import ast
from pathlib import Path
import unittest


ASSET = Path(__file__).resolve().parents[1] / "assets" / "tideglass_soft_rush.py"


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


class TideglassSoftRushTests(unittest.TestCase):
    def test_candidate_has_sixteen_upright_culms_at_realistic_scale(self):
        values, *_ = source_contract()
        self.assertEqual(values["NAME"], "TideglassSoftRush")
        self.assertIn("no generated textures", values["PROVENANCE"])
        self.assertEqual(len(values["STEM_BASES"]), 16)
        self.assertEqual(len(values["STEM_HEIGHTS"]), len(values["STEM_BASES"]))
        self.assertGreater(min(values["STEM_HEIGHTS"]), 0.5)
        self.assertLess(max(values["STEM_HEIGHTS"]), 1.0)
        self.assertEqual(len(set(values["STEM_BASES"])), len(values["STEM_BASES"]))
        self.assertLessEqual(max(abs(value) for base in values["STEM_BASES"]
                                 for value in base), 0.17)
        self.assertLess(values["MIN_CULM_LEAN"], values["MAX_CULM_LEAN"])
        self.assertGreater(values["MID_STEM_CURVE"], 0.0)

    def test_side_panicles_have_distinct_seed_pods_and_two_muted_materials(self):
        values, materials, _collisions, tree = source_contract()
        self.assertEqual(values["SEED_BRANCHES"], 3)
        self.assertEqual(values["SPIKELETS_PER_BRANCH"], 3)
        self.assertTrue({"soft_rush_stem", "soft_rush_seed"}.issubset(materials))
        pod_sphere = next(node for node in ast.walk(tree)
                          if isinstance(node, ast.Call)
                          and isinstance(node.func, ast.Attribute)
                          and node.func.attr == "create_uvsphere")
        kwargs = {item.arg: ast.literal_eval(item.value) for item in pod_sphere.keywords}
        self.assertEqual((kwargs["u_segments"], kwargs["v_segments"]), (5, 3))

    def test_candidate_is_deterministic_and_collisionless(self):
        _values, _materials, collision_calls, tree = source_contract()
        self.assertEqual(collision_calls, [])
        imported = {alias.name for node in tree.body if isinstance(node, ast.Import)
                    for alias in node.names}
        self.assertNotIn("random", imported)


if __name__ == "__main__":
    unittest.main()
