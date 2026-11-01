"""Create PVE-free copies of Norway Maple A's bark and foliage instances.

Run only in the PVE-enabled disposable material sandbox after
Build-MegaplantRuntimeMaterialSandbox.py. Source instances are loaded from the
copied tree package and never modified. The new instances retain all authored
overrides and point at the copied, project-owned tree master material.
"""

import json
import os
import traceback

import unreal


MANIFEST_RELATIVE_PATH = os.path.join("MegaplantRuntimeMaterialSandbox", "copy-manifest.json")
PLUGIN_PREFIXES = ("/ProceduralVegetationEditor/", "/ProceduralVegetation/")
RUNTIME_PARENT = (
    "/Game/Megaplant_Library/RuntimeMaterials/PVE/SampleAssets/Materials/"
    "MasterMaterials/MA_Foliage_Trees.MA_Foliage_Trees"
)
DESTINATION_FOLDER = "/Game/Megaplant_Library/RuntimeMaterials/Species/NorwayMaple"
DESTINATION_PREFIX = DESTINATION_FOLDER + "/"
INSTANCE_MAP = (
    (
        "/Game/Megaplant_Library/Tree_Norway_Maple/Materials/MI_Norway_Maple_Bark_01.MI_Norway_Maple_Bark_01",
        DESTINATION_FOLDER + "/MI_Norway_Maple_Bark_01_Runtime.MI_Norway_Maple_Bark_01_Runtime",
    ),
    (
        "/Game/Megaplant_Library/Tree_Norway_Maple/Materials/MI_Norway_Maple_Foliage_01.MI_Norway_Maple_Foliage_01",
        DESTINATION_FOLDER + "/MI_Norway_Maple_Foliage_01_Runtime.MI_Norway_Maple_Foliage_01_Runtime",
    ),
)


def fail(message):
    raise RuntimeError("[MegaplantRuntimeMaterialSandbox] " + message)


def require_sandbox_manifest():
    project_dir = os.path.realpath(unreal.Paths.project_dir())
    project_name = os.path.basename(os.path.normpath(project_dir))
    project_file = os.path.join(project_dir, "MegaplantRuntimeMaterialSandbox.uproject")
    if project_name.casefold() != "megaplantruntimematerialsandbox" or not os.path.isfile(project_file):
        fail("refusing to create instances outside the dedicated MegaplantRuntimeMaterialSandbox project")
    saved_dir = os.path.realpath(unreal.Paths.project_saved_dir())
    manifest_path = os.path.realpath(os.path.join(saved_dir, MANIFEST_RELATIVE_PATH))
    if os.path.commonpath((saved_dir, manifest_path)) != saved_dir:
        fail("copy manifest path escaped the sandbox Saved directory")
    return manifest_path


def parameter_signature(instance, property_name, value_property):
    values = instance.get_editor_property(property_name) or []
    signature = []
    for entry in values:
        info = entry.get_editor_property("parameter_info")
        name = str(info.get_editor_property("name"))
        value = entry.get_editor_property(value_property)
        if isinstance(value, unreal.Texture):
            value = value.get_path_name() if value else None
        elif isinstance(value, unreal.LinearColor):
            value = [
                float(value.get_editor_property(channel))
                for channel in ("r", "g", "b", "a")
            ]
        elif value is not None:
            value = float(value) if property_name == "scalar_parameter_values" else str(value)
        signature.append({"name": name, "value": value})
    return sorted(signature, key=lambda item: item["name"])


def override_signature(instance):
    return {
        "texture": parameter_signature(instance, "texture_parameter_values", "parameter_value"),
        "scalar": parameter_signature(instance, "scalar_parameter_values", "parameter_value"),
        "vector": parameter_signature(instance, "vector_parameter_values", "parameter_value"),
    }


def main():
    manifest_path = require_sandbox_manifest()
    if not os.path.isfile(manifest_path):
        fail("the PVE graph copy manifest is missing; run the graph builder first")
    with open(manifest_path, "r", encoding="utf-8") as source:
        manifest = json.load(source)
    if manifest.get("count") != len(manifest.get("assets", [])) or manifest.get("count") != 24:
        fail("the PVE graph manifest is empty, inconsistent, or not the verified 24-package closure")
    if manifest.get("norway_maple_material_instances"):
        fail("Norway Maple runtime material instances already exist in the manifest")

    library = unreal.EditorAssetLibrary
    runtime_parent = library.load_asset(RUNTIME_PARENT)
    if not isinstance(runtime_parent, unreal.Material):
        fail("the copied project-owned master material is missing: " + RUNTIME_PARENT)
    if any(runtime_parent.get_path_name().startswith(prefix) for prefix in PLUGIN_PREFIXES):
        fail("runtime parent unexpectedly resolves to plugin content")

    source_instances = []
    for source_path, destination_path in INSTANCE_MAP:
        if library.does_asset_exist(destination_path):
            fail("destination already exists; no instances changed: " + destination_path)
        instance = library.load_asset(source_path)
        if not isinstance(instance, unreal.MaterialInstanceConstant):
            fail("source material instance failed to load: " + source_path)
        source_instances.append((source_path, destination_path, instance, override_signature(instance)))

    if not library.does_directory_exist(DESTINATION_FOLDER):
        if not library.make_directory(DESTINATION_FOLDER):
            fail("could not create sandbox output folder: " + DESTINATION_FOLDER)

    created = []
    for source_path, destination_path, source_instance, before in source_instances:
        if not destination_path.startswith(DESTINATION_PREFIX) or ".." in destination_path.split("/"):
            fail("destination escaped the sandbox species folder: " + destination_path)
        duplicate = library.duplicate_asset(source_path, destination_path)
        if not isinstance(duplicate, unreal.MaterialInstanceConstant):
            fail("could not duplicate source instance: " + source_path)
        unreal.MaterialEditingLibrary.set_material_instance_parent(duplicate, runtime_parent)
        unreal.MaterialEditingLibrary.update_material_instance(duplicate)
        if duplicate.get_editor_property("parent") != runtime_parent:
            fail("runtime parent did not persist on copied instance: " + destination_path)
        after = override_signature(duplicate)
        if before != after:
            fail("authored override values changed while reparenting: " + destination_path)
        if any(destination_path.startswith(prefix) for prefix in PLUGIN_PREFIXES):
            fail("destination unexpectedly points to plugin content: " + destination_path)
        if not library.save_loaded_asset(duplicate, False):
            fail("could not save copied material instance: " + destination_path)
        created.append({
            "source_object": source_path,
            "destination_object": destination_path,
            "parent_object": runtime_parent.get_path_name(),
            "override_counts": {key: len(value) for key, value in after.items()},
        })

    manifest["norway_maple_material_instances"] = created
    temporary_manifest = manifest_path + ".tmp"
    with open(temporary_manifest, "w", encoding="utf-8") as output:
        json.dump(manifest, output, indent=2)
    os.replace(temporary_manifest, manifest_path)
    unreal.log(
        "[MegaplantRuntimeMaterialSandbox] NORWAY_MAPLE_MI_COPY_COMPLETE instances={} overrides_preserved=1 parent={}".format(
            len(created), runtime_parent.get_path_name()
        )
    )


try:
    main()
except Exception:
    unreal.log_error("[MegaplantRuntimeMaterialSandbox] NORWAY_MAPLE_MI_COPY_FAILED " + traceback.format_exc())
    raise
