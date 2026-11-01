"""Keep the PVE material-copy experiment confined to its disposable project."""

import ast
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[2]
BUILD_SCRIPT = ROOT / "Scripts" / "Build-MegaplantRuntimeMaterialSandbox.py"
VERIFY_SCRIPT = ROOT / "Scripts" / "Verify-MegaplantRuntimeMaterialSandbox.py"
PREPARE_SCRIPT = ROOT / "Scripts" / "Prepare-MegaplantNorwayMapleMaterialSandbox.py"
PVE_PROJECT = ROOT / "Playtests" / "MegaplantRuntimeMaterialSandbox" / "MegaplantRuntimeMaterialSandbox.uproject"
NO_PVE_PROJECT = ROOT / "Playtests" / "MegaplantRuntimeMaterialSandbox" / "MegaplantRuntimeMaterialSandbox_NoPVE.uproject"


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
        for script in (BUILD_SCRIPT, PREPARE_SCRIPT, VERIFY_SCRIPT):
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

    def test_instance_prepare_guards_project_before_duplicate_or_reparent(self):
        source = PREPARE_SCRIPT.read_text(encoding="utf-8")
        ast.parse(source, filename=str(PREPARE_SCRIPT))
        self.assertLess(source.index("manifest_path = require_sandbox_manifest()"),
                        source.index("duplicate_asset("))
        self.assertLess(source.index("manifest_path = require_sandbox_manifest()"),
                        source.index("set_material_instance_parent("))
        self.assertIn("if before != after", source)

    def test_no_pve_project_keeps_editor_python_tools_but_disables_pve(self):
        import json

        descriptor = json.loads(NO_PVE_PROJECT.read_text(encoding="utf-8"))
        source_descriptor = json.loads(PVE_PROJECT.read_text(encoding="utf-8"))
        enabled_plugins = {
            plugin["Name"] for plugin in descriptor.get("Plugins", [])
            if plugin.get("Enabled")
        }
        self.assertNotIn("ProceduralVegetationEditor", enabled_plugins)
        self.assertNotIn("ProceduralVegetation", enabled_plugins)
        self.assertIn("PythonScriptPlugin", enabled_plugins)
        self.assertIn("EditorScriptingUtilities", enabled_plugins)
        self.assertIn("DynamicWind", enabled_plugins)
        self.assertIn("ProceduralVegetationEditor", {
            plugin["Name"] for plugin in source_descriptor.get("Plugins", [])
            if plugin.get("Enabled")
        })


if __name__ == "__main__":
    unittest.main()
