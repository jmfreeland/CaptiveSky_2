"""Source contract for the collisionless Tideglass bogbean candidate."""
import ast
from pathlib import Path
import unittest


ASSET = Path(__file__).resolve().parents[1] / "assets" / "tideglass_bogbean.py"


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
    materials = {
        node.targets[0].slice.value
        for node in ast.walk(tree)
        if isinstance(node, ast.Assign)
        and isinstance(node.targets[0], ast.Subscript)
        and isinstance(node.targets[0].value, ast.Attribute)
        and node.targets[0].value.attr == "PLAIN"
        and isinstance(node.targets[0].slice, ast.Constant)
    }
    calls = [node for node in ast.walk(tree) if isinstance(node, ast.Call)]
    collisions = [node for node in calls if isinstance(node.func, ast.Attribute)
                  and node.func.attr.startswith("collide_")]
    return values, materials, collisions, tree


class TideglassBogbeanTests(unittest.TestCase):
    def test_candidate_has_trifoliate_leaves_and_a_visible_flowering_raceme(self):
        values, *_ = source_contract()
        self.assertEqual(values["NAME"], "TideglassBogbean")
        self.assertIn("Menyanthes trifoliata-inspired", values["PROVENANCE"])
        self.assertEqual(values["LEAF_CLUSTERS"], 6)
        self.assertEqual(values["LEAFLET_COUNT_PER_CLUSTER"], 3)
        self.assertEqual(values["FLOWER_STALKS"], 3)
        self.assertEqual(values["FLOWERS_PER_STALK"], (4, 5, 4))
        self.assertEqual(sum(values["FLOWERS_PER_STALK"]), 13)
        self.assertGreater(values["FLOWER_RACEME_START_T"], 0.5)
        self.assertLess(values["FLOWER_RACEME_END_T"], 1.0)
        self.assertGreater(values["FLOWER_RACEME_T_JITTER"], 0.0)
        self.assertEqual(values["FLOWER_SIZE_SCALE_RANGE"], (0.68, 0.98))
        self.assertAlmostEqual(values["FLOWER_DIAMETER_M"], 0.058)
        self.assertGreater(values["FLOWER_HEIGHTS_M"][1], values["LEAFLET_LENGTH_M"] * 3)

    def test_candidate_is_deterministic_collisionless_and_uses_readable_material_groups(self):
        _values, materials, collisions, tree = source_contract()
        self.assertEqual(collisions, [])
        imported = {alias.name for node in tree.body if isinstance(node, ast.Import)
                    for alias in node.names}
        self.assertNotIn("random", imported)
        self.assertNotIn("secrets", imported)
        self.assertTrue({
            "bogbean_leaf", "bogbean_leaf_light", "bogbean_petioles",
            "bogbean_flower", "bogbean_flower_blush", "bogbean_fringe",
            "bogbean_centre",
        }.issubset(materials))

    def test_builder_has_low_waterline_foliage_and_fringed_five_lobed_flowers(self):
        *_rest, tree = source_contract()
        functions = {node.name: node for node in tree.body if isinstance(node, ast.FunctionDef)}
        self.assertTrue({"_oriented_ellipsoid", "_fringe_filament", "_add_flower", "build"}.issubset(functions))
        flower_source = ast.unparse(functions["_add_flower"])
        builder_source = ast.unparse(functions["build"])
        self.assertIn("range(FLOWER_PETAL_COUNT)", flower_source)
        self.assertIn("_fringe_filament(fringe", flower_source)
        self.assertIn("size_scale", flower_source)
        self.assertIn("radial * 0.11", builder_source)
        self.assertIn("bogbeanFlowerScape", builder_source)
        self.assertIn("bogbeanPetiole", builder_source)
        self.assertIn("bogbeanPetalFringes", builder_source)
        self.assertIn("math.sin(phase", builder_source)
        self.assertIn("math.cos(phase", builder_source)
        self.assertIn("triangulate", builder_source)


if __name__ == "__main__":
    unittest.main()
