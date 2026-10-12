"""Migrate the project's PlantFactory billboard calls onto local copies.

This script changes four known material packages only. It refuses to run unless
CAPTIVESKY_APPLY_PLANTFACTORY_MIGRATION=1 is present in the process environment.
Back up the packages before invoking it. The project descriptor is not changed.
"""

import json
import os
import traceback

import unreal


FUNCTION_ROOT = "/Game/Generated/PlantMaterialCompatibility"
MATERIALS = (
    ("Festuca", "/Game/Plants/Materials/Festuca_gautieri_LD/MM_Festuca_gautieri_LD", 4),
    ("Phalaris", "/Game/Plants/Materials/Phalaris_arundinacea_LD/MM_Phalaris_arundinacea_LD", 4),
    ("Typha", "/Game/Plants/Materials/Typha_latifolia_LD/MM_Typha_latifolia_LD", 4),
    ("RhododendronRepairTest", "/Game/Generated/MaterialTests/Codex_RhodoMaterialRepair_20261012/MM_Rhodo_InstancedRepair_Test", 4),
)


def fail(message):
    raise RuntimeError(message)


def main():
    if os.environ.get("CAPTIVESKY_APPLY_PLANTFACTORY_MIGRATION") != "1":
        fail("Set CAPTIVESKY_APPLY_PLANTFACTORY_MIGRATION=1 after backing up the source packages.")

    replacements = {}
    for name in ("StaticBillboard", "BillboardMappedNormals"):
        path = FUNCTION_ROOT + "/" + name + "." + name
        function = unreal.EditorAssetLibrary.load_asset(path)
        if not isinstance(function, unreal.MaterialFunction):
            fail("Project-local MaterialFunction missing: " + path)
        replacements["/PlantFactoryPlugin/{}.{}".format(name, name)] = function

    report = {"materials": [], "errors": []}
    for label, asset_path, expected_count in MATERIALS:
        material = unreal.EditorAssetLibrary.load_asset(asset_path)
        if not isinstance(material, unreal.Material):
            fail("Material missing: " + asset_path)

        remapped = []
        remaining = []
        for expression in unreal.MaterialEditingLibrary.get_material_expressions(material):
            if not isinstance(expression, unreal.MaterialExpressionMaterialFunctionCall):
                continue
            function = expression.get_editor_property("material_function")
            if not function:
                continue
            old_path = function.get_path_name()
            replacement = replacements.get(old_path)
            if replacement:
                expression.set_editor_property("material_function", replacement)
                remapped.append({"node": expression.get_name(), "from": old_path,
                                 "to": replacement.get_path_name()})
            current = expression.get_editor_property("material_function")
            if current and current.get_path_name().startswith("/PlantFactoryPlugin/"):
                remaining.append(current.get_path_name())

        if len(remapped) != expected_count:
            fail("Unexpected remap count for {}: expected {}, got {}".format(
                label, expected_count, len(remapped)))
        if remaining:
            fail("Unmigrated plugin functions in {}: {}".format(label, remaining))

        diagnostics = unreal.MaterialEditingLibrary.recompile_material(material)
        if diagnostics:
            fail("Material recompile diagnostics for {}: {}".format(label, diagnostics))
        if not unreal.EditorAssetLibrary.save_loaded_asset(material):
            fail("Could not save migrated material: " + asset_path)
        report["materials"].append({
            "label": label,
            "asset": material.get_path_name(),
            "remapped_calls": remapped,
            "remaining_plugin_calls": remaining,
            "recompile_result": list(diagnostics),
        })

    unreal.log("[PlantFactoryMigrationApply] REPORT " + json.dumps(report, sort_keys=True))
    unreal.log("[PlantFactoryMigrationApply] COMPLETE materials={} remapped_calls={}".format(
        len(report["materials"]), sum(len(item["remapped_calls"]) for item in report["materials"])))


try:
    main()
except Exception:
    unreal.log_error("[PlantFactoryMigrationApply] FAILED " + traceback.format_exc())
    raise
