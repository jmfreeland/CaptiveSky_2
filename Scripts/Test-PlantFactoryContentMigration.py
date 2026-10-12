"""Exercise a reversible PlantFactory function migration on disposable copies only.

Run with UE 5.8.3 UnrealEditor-Cmd and -NullRHI. The script duplicates the two
function assets used by Island vegetation plus four master-material test copies,
rewires only those copies, recompiles them, and saves the test assets. It does
not edit source materials, the Island map, or the project descriptor.
"""

import json
import traceback

import unreal


FUNCTIONS = ("StaticBillboard", "BillboardMappedNormals")
FUNCTION_ROOT = "/Game/Generated/PlantMaterialCompatibility"
TRIAL_ROOT = "/Game/Generated/MaterialTests/PlantFactoryMigrationTrial_20261012"
MATERIALS = (
    ("Festuca", "/Game/Plants/Materials/Festuca_gautieri_LD/MM_Festuca_gautieri_LD"),
    ("Phalaris", "/Game/Plants/Materials/Phalaris_arundinacea_LD/MM_Phalaris_arundinacea_LD"),
    ("Typha", "/Game/Plants/Materials/Typha_latifolia_LD/MM_Typha_latifolia_LD"),
    ("RhododendronTest", "/Game/Generated/MaterialTests/Codex_RhodoMaterialRepair_20261012/MM_Rhodo_InstancedRepair_Test"),
)


def fail(message):
    raise RuntimeError(message)


def main():
    if unreal.EditorAssetLibrary.does_directory_exist(FUNCTION_ROOT):
        fail("Refusing to reuse existing migration function folder: " + FUNCTION_ROOT)
    if unreal.EditorAssetLibrary.does_directory_exist(TRIAL_ROOT):
        fail("Refusing to reuse existing migration trial folder: " + TRIAL_ROOT)

    report = {"functions": [], "materials": [], "errors": []}
    replacements = {}
    unreal.EditorAssetLibrary.make_directory(FUNCTION_ROOT)
    for name in FUNCTIONS:
        source_path = "/PlantFactoryPlugin/{0}.{0}".format(name)
        destination_path = FUNCTION_ROOT + "/" + name
        source = unreal.EditorAssetLibrary.load_asset(source_path)
        if not isinstance(source, unreal.MaterialFunction):
            fail("Source MaterialFunction missing: " + source_path)
        copied = unreal.EditorAssetLibrary.duplicate_asset(source_path, destination_path)
        if not isinstance(copied, unreal.MaterialFunction):
            fail("Could not duplicate MaterialFunction: " + source_path)
        if not unreal.EditorAssetLibrary.save_loaded_asset(copied):
            fail("Could not save copied MaterialFunction: " + destination_path)
        replacements[source.get_path_name()] = copied
        report["functions"].append({"source": source.get_path_name(), "copy": copied.get_path_name()})

    for label, source_path in MATERIALS:
        source = unreal.EditorAssetLibrary.load_asset(source_path)
        if not isinstance(source, unreal.Material):
            fail("Source Material missing: " + source_path)
        destination_folder = TRIAL_ROOT + "/" + label
        unreal.EditorAssetLibrary.make_directory(destination_folder)
        destination_path = destination_folder + "/" + source.get_name() + "_MigrationTest"
        copied = unreal.EditorAssetLibrary.duplicate_asset(source_path, destination_path)
        if not isinstance(copied, unreal.Material):
            fail("Could not duplicate Material: " + source_path)

        remapped = []
        unresolved = []
        for expression in unreal.MaterialEditingLibrary.get_material_expressions(copied):
            if not isinstance(expression, unreal.MaterialExpressionMaterialFunctionCall):
                continue
            function = expression.get_editor_property("material_function")
            if not function:
                continue
            replacement = replacements.get(function.get_path_name())
            if replacement:
                expression.set_editor_property("material_function", replacement)
                remapped.append({"node": expression.get_name(), "from": function.get_path_name(),
                                 "to": replacement.get_path_name()})
            current = expression.get_editor_property("material_function")
            if current and current.get_path_name().startswith("/PlantFactoryPlugin/"):
                unresolved.append(current.get_path_name())
        if unresolved:
            fail("Unmigrated plugin function references in {}: {}".format(label, unresolved))

        compile_result = unreal.MaterialEditingLibrary.recompile_material(copied)
        if compile_result:
            fail("Material recompile reported diagnostics for {}: {}".format(label, compile_result))
        if not unreal.EditorAssetLibrary.save_loaded_asset(copied):
            fail("Could not save trial material: " + destination_path)
        report["materials"].append({
            "label": label,
            "source": source.get_path_name(),
            "trial": copied.get_path_name(),
            "remapped_calls": remapped,
            "unresolved_plugin_calls": unresolved,
            "recompile_result": list(compile_result),
        })

    unreal.log("[PlantFactoryMigrationTrial] REPORT " + json.dumps(report, sort_keys=True))
    unreal.log("[PlantFactoryMigrationTrial] COMPLETE functions={} materials={} remapped_calls={}".format(
        len(report["functions"]), len(report["materials"]),
        sum(len(item["remapped_calls"]) for item in report["materials"]),
    ))


try:
    main()
except Exception:
    unreal.log_error("[PlantFactoryMigrationTrial] FAILED " + traceback.format_exc())
    raise
