"""Read-only verification with the PlantFactory content mount disabled."""

import json
import os
import traceback

import unreal


MAP_PATH = "/Game/Maps/Island"
MATERIALS = (
    "/Game/Plants/Materials/Festuca_gautieri_LD/MM_Festuca_gautieri_LD",
    "/Game/Plants/Materials/Phalaris_arundinacea_LD/MM_Phalaris_arundinacea_LD",
    "/Game/Plants/Materials/Typha_latifolia_LD/MM_Typha_latifolia_LD",
    "/Game/Generated/MaterialTests/Codex_RhodoMaterialRepair_20261012/MM_Rhodo_InstancedRepair_Test",
)


def fail(message):
    raise RuntimeError(message)


def main():
    project_directory = unreal.SystemLibrary.get_project_directory()
    descriptor_path = os.path.join(project_directory, "CaptiveSky_2.uproject")
    with open(descriptor_path, "r", encoding="utf-8") as descriptor_file:
        descriptor = json.load(descriptor_file)
    enabled = [entry for entry in descriptor.get("Plugins", [])
               if entry.get("Name") == "PlantFactoryPlugin" and entry.get("Enabled", False)]
    if enabled:
        fail("PlantFactoryPlugin is still enabled in the project descriptor.")

    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.wait_for_completion()
    plugin_assets = registry.get_assets_by_path("/PlantFactoryPlugin", recursive=True) or []

    # A cached Asset Registry can retain entries from disabled content mounts;
    # plugin load state is verified from the .uproject and startup log instead.
    report = {"plugin_enabled": False, "cached_plugin_asset_entries": len(plugin_assets),
              "materials": [], "island_loaded": False}
    for asset_path in MATERIALS:
        material = unreal.EditorAssetLibrary.load_asset(asset_path)
        if not isinstance(material, unreal.Material):
            fail("Could not load migrated material with plugin disabled: " + asset_path)
        plugin_calls = []
        for expression in unreal.MaterialEditingLibrary.get_material_expressions(material):
            if not isinstance(expression, unreal.MaterialExpressionMaterialFunctionCall):
                continue
            function = expression.get_editor_property("material_function")
            if function and function.get_path_name().startswith("/PlantFactoryPlugin/"):
                plugin_calls.append(function.get_path_name())
        if plugin_calls:
            fail("Material still calls PlantFactory functions: {} -> {}".format(asset_path, plugin_calls))
        diagnostics = unreal.MaterialEditingLibrary.recompile_material(material)
        if diagnostics:
            fail("Material compile diagnostics with plugin disabled: {} -> {}".format(asset_path, diagnostics))
        report["materials"].append({"asset": material.get_path_name(), "plugin_calls": plugin_calls,
                                    "compile_diagnostics": list(diagnostics)})

    if not unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH):
        fail("Could not load Island with PlantFactoryPlugin disabled.")
    report["island_loaded"] = bool(unreal.EditorLevelLibrary.get_editor_world())
    if not report["island_loaded"]:
        fail("Island reported successful load but no editor world was available.")

    unreal.log("[PlantFactoryMigrationVerify] REPORT " + json.dumps(report, sort_keys=True))
    unreal.log("[PlantFactoryMigrationVerify] COMPLETE materials={} island_loaded={}".format(
        len(report["materials"]), report["island_loaded"]))


try:
    main()
except Exception:
    unreal.log_error("[PlantFactoryMigrationVerify] FAILED " + traceback.format_exc())
    raise
