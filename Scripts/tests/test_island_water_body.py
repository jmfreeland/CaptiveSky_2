"""Regression checks for the Island water-body builder."""

import ast
from pathlib import Path
import unittest


BUILDER = Path(__file__).resolve().parents[1] / "Create-IslandWaterBody.py"


def hidden_plane_collision_calls():
    tree = ast.parse(BUILDER.read_text(encoding="utf-8"), filename=str(BUILDER))
    calls = []
    for node in ast.walk(tree):
        if not isinstance(node, ast.If):
            continue
        condition = ast.unparse(node.test)
        if "PLANE_NAME" not in condition or "StaticMeshActor" not in condition:
            continue
        for child in ast.walk(node):
            if not isinstance(child, ast.Call) or not isinstance(child.func, ast.Attribute):
                continue
            if child.func.attr == "set_collision_enabled":
                calls.append(child)
    return calls


class IslandWaterBodyBuilderTests(unittest.TestCase):
    def test_hidden_legacy_plane_is_non_colliding(self):
        calls = hidden_plane_collision_calls()
        self.assertEqual(len(calls), 1, "the retained legacy ocean plane needs one explicit collision policy")
        self.assertEqual(ast.unparse(calls[0].args[0]), "unreal.CollisionEnabled.NO_COLLISION")

    def test_collision_policy_is_saved_synchronously_during_build(self):
        tree = ast.parse(BUILDER.read_text(encoding="utf-8"), filename=str(BUILDER))
        build = next(node for node in tree.body if isinstance(node, ast.FunctionDef) and node.name == "build")
        save_calls = [
            node for node in ast.walk(build)
            if isinstance(node, ast.Call)
            and isinstance(node.func, ast.Attribute)
            and node.func.attr in {"save_current_level", "save_dirty_packages"}
        ]
        self.assertEqual(
            {node.func.attr for node in save_calls},
            {"save_current_level", "save_dirty_packages"},
            "the collision change must be persisted before deferred post-tick work",
        )


if __name__ == "__main__":
    unittest.main()
