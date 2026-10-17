"""Source-level checks for the collisionless common sea-lavender candidate."""
import ast
from pathlib import Path
import unittest


ASSET = Path(__file__).resolve().parents[1] / "assets" / "coastal_sea_lavender.py"
IMPORTER = Path(__file__).resolve().parents[2] / "Import-ComfyBlenderAsset.py"


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
    collisions = [node for node in calls if isinstance(node.func, ast.Attribute)
                  and node.func.attr.startswith("collide_")]
    rng_imports = [node for node in tree.body if isinstance(node, ast.Import)
                   and any(alias.name == "random" for alias in node.names)]
    return values, tree, calls, collisions, rng_imports


def plain_material_contract(tree):
    materials = {}
    for node in ast.walk(tree):
        if not isinstance(node, ast.Assign):
            continue
        for target in node.targets:
            if (isinstance(target, ast.Subscript)
                    and isinstance(target.value, ast.Attribute)
                    and target.value.attr == "PLAIN"
                    and isinstance(target.slice, ast.Constant)):
                materials[target.slice.value] = ast.literal_eval(node.value)
    return materials


class CoastalSeaLavenderTests(unittest.TestCase):
    def test_candidate_has_airborne_sprays_and_low_basal_rosette(self):
        values, *_ = source_contract()
        self.assertEqual(values["NAME"], "CoastalSeaLavender")
        self.assertIn("Limonium vulgare-inspired", values["PROVENANCE"])
        self.assertEqual(len(values["ROSETTE_CENTRES"]), 6)
        self.assertEqual(len(values["FLOWER_STEMS"]), 8)
        self.assertEqual(values["ROSETTE_LEAVES"], 6)
        self.assertEqual(values["BRANCHES_PER_STEM"], 3)
        self.assertEqual(values["FLOWERS_PER_BRANCH"], 4)
        self.assertEqual(values["PETALS_PER_FLOWER"], 5)
        self.assertGreater(min(stem[2] for stem in values["FLOWER_STEMS"]), 0.4)
        self.assertLess(max(stem[2] for stem in values["FLOWER_STEMS"]), 0.6)

    def test_geometry_is_seedless_repeatable_and_has_no_collision(self):
        values, tree, calls, collisions, rng_imports = source_contract()
        self.assertEqual(collisions, [])
        self.assertEqual(rng_imports, [])
        tube_calls = [node for node in calls if isinstance(node.func, ast.Attribute)
                      and node.func.attr == "tube"]
        self.assertEqual(len(tube_calls), 2)
        self.assertTrue(all(call.args and isinstance(call.args[0], ast.Constant)
                            and call.args[0].value == "sea_lavender_stem"
                            for call in tube_calls))
        self.assertEqual(sorted(ast.literal_eval(call.args[2]) for call in tube_calls),
                         [0.0026, 0.0042])
        petal_function = next(node for node in tree.body if isinstance(node, ast.FunctionDef)
                              and node.name == "_star_floret")
        self.assertIn("PETALS_PER_FLOWER", ast.unparse(petal_function))
        self.assertIn("0.014", ast.unparse(tree))
        self.assertIn("no generated textures", values["PROVENANCE"])

    def test_candidate_keeps_default_shading_but_importer_supports_opt_in(self):
        _values, tree, *_ = source_contract()
        materials = plain_material_contract(tree)
        self.assertTrue(all("TwoSided" not in material for material in materials.values()))
        self.assertLess(max(materials["sea_lavender_leaf"]["BaseColor"]), 0.3)
        self.assertGreater(min(materials["sea_lavender_stem"]["BaseColor"]), 0.25)
        self.assertGreater(materials["sea_lavender_flower"]["BaseColor"][2], 0.8)

        importer = IMPORTER.read_text(encoding="utf-8")
        self.assertIn('description.get("TwoSided", False)', importer)
        self.assertIn('"material_properties"', importer)
        self.assertIn('material.get_editor_property("two_sided")', importer)


if __name__ == "__main__":
    unittest.main()
