"""Keep the PVE material-copy experiment confined to its disposable project."""

import ast
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
BUILD_SCRIPT = ROOT / "Scripts" / "Build-MegaplantRuntimeMaterialSandbox.py"
VERIFY_SCRIPT = ROOT / "Scripts" / "Verify-MegaplantRuntimeMaterialSandbox.py"


class MegaplantRuntimeMaterialSandboxTests(unittest.TestCase):
    def test_copy_script_guards_project_before_asset_registry_or_copy(self):
        source = BUILD_SCRIPT.read_text(encoding="utf-8")
        self.assertLess(source.index("manifest_path = require_sandbox_project()"),
                        source.index("AssetRegistryHelpers.get_asset_registry()"))
        self.assertLess(source.index("manifest_path = require_sandbox_project()"),
                        source.index("duplicate_asset("))
        self.assertIn('"MegaplantRuntimeMaterialSandbox.uproject"', source)
        self.assertIn("os.path.commonpath((saved_dir, manifest_path))", source)

    def test_manifest_is_rooted_in_project_saved_for_both_scripts(self):
        for script in (BUILD_SCRIPT, VERIFY_SCRIPT):
            with self.subTest(script=script.name):
                source = script.read_text(encoding="utf-8")
                ast.parse(source, filename=str(script))
                self.assertIn("unreal.Paths.project_saved_dir()", source)
                self.assertIn("MegaplantRuntimeMaterialSandbox.uproject", source)
                self.assertIn("os.path.commonpath((saved_dir, manifest_path))", source)

    def test_verify_script_guards_project_before_opening_the_manifest(self):
        source = VERIFY_SCRIPT.read_text(encoding="utf-8")
        self.assertLess(source.index("manifest_path = require_sandbox_manifest()"),
                        source.index('with open(manifest_path, "r"'))


if __name__ == "__main__":
    unittest.main()
