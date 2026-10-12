"""Source contracts for the local-only Aster bread practice prop."""
import ast
import importlib.util
from pathlib import Path
import unittest


PIPE = Path(__file__).resolve().parents[1]
ASSET = PIPE / "assets" / "bread.py"
LIB = PIPE / "lib.py"
TEXTURE_BUILDER = PIPE / "make_bread_texture.py"

SPEC = importlib.util.spec_from_file_location("make_bread_texture", TEXTURE_BUILDER)
TEXTURE_MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(TEXTURE_MODULE)


def calls_in_build():
    tree = ast.parse(ASSET.read_text(encoding="utf-8"), filename=str(ASSET))
    build = next(node for node in tree.body
                 if isinstance(node, ast.FunctionDef) and node.name == "build")
    calls = []
    for node in ast.walk(build):
        if not isinstance(node, ast.Call) or not isinstance(node.func, ast.Attribute):
            continue
        if isinstance(node.func.value, ast.Name) and node.func.value.id == "A":
            calls.append(node)
    return calls


def keyword(call, name):
    item = next(value for value in call.keywords if value.arg == name)
    return ast.literal_eval(item.value)


class BreadPracticeAssetTests(unittest.TestCase):
    def test_materials_and_small_board_collision_are_registered(self):
        calls = calls_in_build()
        lib_tree = ast.parse(LIB.read_text(encoding="utf-8"), filename=str(LIB))
        material_keys = set()
        for node in lib_tree.body:
            if isinstance(node, ast.Assign) and any(
                isinstance(target, ast.Name) and target.id in {"PLAIN", "TILED"}
                for target in node.targets
            ) and isinstance(node.value, ast.Dict):
                material_keys.update(ast.literal_eval(key) for key in node.value.keys)
        asset_tree = ast.parse(ASSET.read_text(encoding="utf-8"), filename=str(ASSET))
        for node in ast.walk(asset_tree):
            if not isinstance(node, ast.Assign):
                continue
            for target in node.targets:
                if (isinstance(target, ast.Subscript)
                        and isinstance(target.value, ast.Attribute)
                        and isinstance(target.value.value, ast.Name)
                        and target.value.value.id == "lib"
                        and target.value.attr in {"TILED", "PLAIN"}
                        and isinstance(target.slice, ast.Constant)
                        and isinstance(target.slice.value, str)):
                    material_keys.add(target.slice.value)

        asset_keys = {
            ast.literal_eval(call.args[0]) for call in calls
            if call.args and isinstance(call.args[0], ast.Constant)
            and isinstance(call.args[0].value, str)
            and isinstance(call.func.value, ast.Name) and call.func.value.id == "A"
            and call.func.attr in {"box", "rock", "tube", "cyl", "torus", "cloth"}
        }
        self.assertTrue(asset_keys)
        self.assertLessEqual(asset_keys, material_keys)

        collisions = [call for call in calls if call.func.attr == "collide_box"]
        self.assertEqual(len(collisions), 1)
        self.assertEqual(ast.literal_eval(collisions[0].args[0]), (0.54, 0.36, 0.025))
        self.assertEqual(keyword(collisions[0], "loc"), (0, 0, 0.0125))

    def test_prop_has_fine_scores_crumb_pores_folded_linen_and_crumbs(self):
        source = ASSET.read_text(encoding="utf-8")
        calls = calls_in_build()
        attrs = [call.func.attr for call in calls]
        self.assertIn("rock", attrs)
        self.assertIn("cyl", attrs)
        self.assertIn("torus", attrs)
        self.assertIn("cloth", attrs)
        self.assertEqual(source.count('A.tube("bread_score"'), 1)
        self.assertIn('for index, lateral in enumerate((-0.026, 0.0, 0.026))', source)
        self.assertIn('tile=6.0', source)
        self.assertEqual(source.count('A.cyl("bread_pore"'), 1)
        self.assertIn('range(5)', source)
        self.assertIn('tile=1.4', source)
        self.assertTrue(any(
            call.func.attr == "cyl" and call.args
            and isinstance(call.args[0], ast.Constant)
            and call.args[0].value == "bread_crumb"
            for call in calls
        ), "the cut face uses warm crumb rather than the pale mushroom placeholder")
        self.assertIn('y = -0.17 + v * 0.11', source)
        self.assertIn('Vector((x, y, 0.030 + ridge + ripple))', source)
        self.assertGreaterEqual(source.count('A.rock("mushstem"'), 1)

    def test_script_is_python_syntax_valid(self):
        ast.parse(ASSET.read_text(encoding="utf-8"), filename=str(ASSET))

    def test_crust_maps_are_deterministic_tileable_and_orm_packed(self):
        first = TEXTURE_MODULE.make_maps(size=128, seed=17)
        second = TEXTURE_MODULE.make_maps(size=128, seed=17)
        for left, right in zip(first, second):
            self.assertEqual(left.shape, (128, 128, 3))
            self.assertTrue((left == right).all())
            for edge, interior in (
                (abs(left[0].astype(float) - left[-1].astype(float)).mean(),
                 abs(left[1:].astype(float) - left[:-1].astype(float)).mean()),
                (abs(left[:, 0].astype(float) - left[:, -1].astype(float)).mean(),
                 abs(left[:, 1:].astype(float) - left[:, :-1].astype(float)).mean()),
            ):
                self.assertLessEqual(float(edge), float(interior) * 1.2 + 1.0)
        albedo, normal, orm = first
        self.assertGreater(float(albedo.std()), 10.0)
        self.assertGreater(float(normal[:, :, 2].mean()), 220.0)
        self.assertTrue((orm[:, :, 2] == 0).all(), "bread is non-metallic")
        self.assertGreater(float(orm[:, :, 1].mean()), 170.0)


if __name__ == "__main__":
    unittest.main()
