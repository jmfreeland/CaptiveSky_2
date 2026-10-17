"""Source-level checks for the collisionless common sea-lavender candidate."""
import ast
from pathlib import Path
import unittest


ASSET = Path(__file__).resolve().parents[1] / "assets" / "coastal_sea_lavender.py"


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


class CoastalSeaLavenderTests(unittest.TestCase):
    def test_candidate_has_airborne_sprays_and_low_basal_rosette(self):
        values, *_ = source_contract()
        self.assertEqual(values["NAME"], "CoastalSeaLavender")
        self.assertIn("Limonium vulgare-inspired", values["PROVENANCE"])
        self.assertEqual(len(values["ROSETTE_CENTRES"]), 6)
        self.assertEqual(len(values["FLOWER_STEMS"]), 8)
        self.assertEqual(values["ROSETTE_LEAVES"], 7)
        self.assertEqual(values["BRANCHES_PER_STEM"], 4)
        self.assertEqual(values["FLOWERS_PER_BRANCH"], 5)
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
        petal_function = next(node for node in tree.body if isinstance(node, ast.FunctionDef)
                              and node.name == "_star_floret")
        self.assertIn("PETALS_PER_FLOWER", ast.unparse(petal_function))
        self.assertIn("no generated textures", values["PROVENANCE"])


if __name__ == "__main__":
    unittest.main()
