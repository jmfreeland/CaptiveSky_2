"""Source-level checks for the collisionless sea-holly dune candidate."""
import ast
from pathlib import Path
import unittest


ASSET = Path(__file__).resolve().parents[1] / "assets" / "coastal_sea_holly.py"


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
    materials = {
        node.targets[0].slice.value
        for node in ast.walk(tree)
        if isinstance(node, ast.Assign)
        and isinstance(node.targets[0], ast.Subscript)
        and isinstance(node.targets[0].value, ast.Attribute)
        and node.targets[0].value.attr == "PLAIN"
        and isinstance(node.targets[0].slice, ast.Constant)
    }
    collisions = [node for node in calls if isinstance(node.func, ast.Attribute)
                  and node.func.attr.startswith("collide_")]
    rng_imports = [node for node in tree.body if isinstance(node, ast.Import)
                   and any(alias.name == "random" for alias in node.names)]
    return values, tree, calls, materials, collisions, rng_imports


class CoastalSeaHollyTests(unittest.TestCase):
    def test_candidate_has_low_spiny_rosette_and_distinct_blue_flower_heads(self):
        values, *_ = source_contract()
        self.assertEqual(values["NAME"], "CoastalSeaHolly")
        self.assertIn("Eryngium maritimum-inspired", values["PROVENANCE"])
        self.assertEqual(values["ROSETTE_LEAVES"], 5)
        self.assertEqual(len(values["LEAF_LENGTHS"]), values["ROSETTE_LEAVES"])
        self.assertEqual(len(values["FLOWER_HEADS"]), 4)
        self.assertEqual(values["BRACTS_PER_HEAD"], 20)
        self.assertGreater(min(head[2] for head in values["FLOWER_HEADS"]), 0.3)
        self.assertLess(max(head[2] for head in values["FLOWER_HEADS"]), 0.5)

    def test_geometry_is_seedless_collisionless_and_has_prickled_lobes(self):
        values, tree, calls, _materials, collisions, rng_imports = source_contract()
        self.assertEqual(collisions, [])
        self.assertEqual(rng_imports, [])
        self.assertEqual(values["SPINY_STATIONS"], (2, 5, 8))
        self.assertEqual(values["LEAF_STATIONS"], 11)
        tubes = [node for node in calls if isinstance(node.func, ast.Attribute)
                 and node.func.attr == "tube"]
        self.assertEqual(len(tubes), 1)
        head = next(node for node in tree.body if isinstance(node, ast.FunctionDef)
                    and node.name == "_bract_head")
        self.assertIn("BRACTS_PER_HEAD", ast.unparse(head))
        self.assertIn("no generated textures", values["PROVENANCE"])

    def test_materials_keep_muted_dune_palette_and_simple_default_shading(self):
        _values, _tree, _calls, materials, *_ = source_contract()
        self.assertEqual(materials, {"sea_holly_leaf", "sea_holly_stem", "sea_holly_bract"})
        _values, tree, *_ = source_contract()
        for node in ast.walk(tree):
            if (isinstance(node, ast.Assign) and isinstance(node.targets[0], ast.Subscript)
                    and isinstance(node.targets[0].value, ast.Attribute)
                    and node.targets[0].value.attr == "PLAIN"):
                self.assertNotIn("TwoSided", ast.unparse(node.value))


if __name__ == "__main__":
    unittest.main()
