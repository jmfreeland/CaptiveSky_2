"""Copy and rewire the audited PVE material closure in the disposable sandbox project.

Run only with Playtests/MegaplantRuntimeMaterialSandbox.uproject and PVE enabled.
The script fails before copying if any source or destination is ambiguous, and
never writes outside the sandbox project's /Game/Megaplant_Library/RuntimeMaterials.
"""

import json
import os
import unreal


PLUGIN_PREFIX = "/ProceduralVegetationEditor/"
DESTINATION_ROOT = "/Game/Megaplant_Library/RuntimeMaterials/PVE"
MANIFEST_RELATIVE_PATH = os.path.join("MegaplantRuntimeMaterialSandbox", "copy-manifest.json")
SOURCE_PACKAGES = (
    "/ProceduralVegetationEditor/SampleAssets/Materials/MSVTTextures/DefaultDiffuse_VT",
    "/ProceduralVegetationEditor/SampleAssets/Materials/MSVTTextures/Leaf_Mask_VT",
    "/ProceduralVegetationEditor/SampleAssets/Materials/MSVTTextures/Mask_VT",
    "/ProceduralVegetationEditor/SampleAssets/Materials/MasterMaterials/MA_Foliage_Trees",
    "/ProceduralVegetationEditor/SampleAssets/Materials/MaterialFunctions/MF_DefaultLit_Trunk",
    "/ProceduralVegetationEditor/SampleAssets/Materials/MaterialFunctions/MF_Displacement",
    "/ProceduralVegetationEditor/SampleAssets/Materials/MaterialFunctions/MF_GenerateBranchMask_MS",
    "/ProceduralVegetationEditor/SampleAssets/Materials/MaterialFunctions/MF_HealthSeason",
    "/ProceduralVegetationEditor/SampleAssets/Materials/MaterialFunctions/MF_MegaPlantAttributes",
    "/ProceduralVegetationEditor/SampleAssets/Materials/MaterialFunctions/MF_NoiseMask",
    "/ProceduralVegetationEditor/SampleAssets/Materials/MaterialFunctions/MF_PerInstanceActorRandom",
    "/ProceduralVegetationEditor/SampleAssets/Materials/MaterialFunctions/MF_ReconstructZ",
    "/ProceduralVegetationEditor/SampleAssets/Materials/MaterialFunctions/MF_Roughness",
    "/ProceduralVegetationEditor/SampleAssets/Materials/MaterialFunctions/MF_TwoSidedRoughness",
    "/ProceduralVegetationEditor/SampleAssets/Materials/MaterialFunctions/MF_TwoSided_Leaves",
    "/ProceduralVegetationEditor/SampleAssets/Materials/MaterialFunctions/MF_adjustAO",
    "/ProceduralVegetationEditor/SampleAssets/Materials/MaterialFunctions/MF_adjustBaseColor",
    "/ProceduralVegetationEditor/SampleAssets/Materials/MaterialFunctions/MF_adjustNormal",
    "/ProceduralVegetationEditor/SampleAssets/Materials/MaterialFunctions/MF_adjustTwoSidedBaseColor",
    "/ProceduralVegetationEditor/SampleAssets/Materials/MaterialFunctions/MF_adjustTwoSidedNormal",
    "/ProceduralVegetationEditor/SampleAssets/Materials/MaterialFunctions/MF_generateSpecular",
    "/ProceduralVegetationEditor/SampleAssets/Materials/MaterialFunctions/MF_generateTranslucency",
    "/ProceduralVegetationEditor/SampleAssets/Materials/MaterialFunctions/MF_generateTwoSidedSpecular",
    "/ProceduralVegetationEditor/SampleAssets/Materials/MaterialFunctions/MPC_GlobalFoliageActor",
)


def object_path(asset):
    return asset.get_path_name() if asset else None


def class_path(asset_data):
    path = asset_data.get_editor_property("asset_class_path")
    return "{}.{}".format(
        path.get_editor_property("package_name"),
        path.get_editor_property("asset_name"),
    )


def package_of(object_path_value):
    return object_path_value.split(".", 1)[0]


def fail(message):
    raise RuntimeError("[MegaplantRuntimeMaterialSandbox] " + message)


def require_sandbox_project():
    project_dir = os.path.realpath(unreal.Paths.project_dir())
    project_name = os.path.basename(os.path.normpath(project_dir))
    project_file = os.path.join(project_dir, "MegaplantRuntimeMaterialSandbox.uproject")
    if project_name.casefold() != "megaplantruntimematerialsandbox" or not os.path.isfile(project_file):
        fail("refusing to copy assets outside the dedicated MegaplantRuntimeMaterialSandbox project")
    saved_dir = os.path.realpath(unreal.Paths.project_saved_dir())
    manifest_path = os.path.realpath(os.path.join(saved_dir, MANIFEST_RELATIVE_PATH))
    if os.path.commonpath((saved_dir, manifest_path)) != saved_dir:
        fail("copy manifest path escaped the sandbox Saved directory")
    return manifest_path


def destination_for(source_package, asset_name):
    if not source_package.startswith(PLUGIN_PREFIX):
        fail("unexpected source package: " + source_package)
    relative_package = source_package[len(PLUGIN_PREFIX):].strip("/")
    destination_package = DESTINATION_ROOT + "/" + relative_package
    return destination_package + "." + asset_name


def references_for(asset):
    if isinstance(asset, unreal.Material):
        expressions = unreal.MaterialEditingLibrary.get_material_expressions(asset)
    elif isinstance(asset, unreal.MaterialFunction):
        expressions = unreal.MaterialEditingLibrary.get_material_function_expressions(asset)
    else:
        return []

    references = []
    for expression in expressions:
        if isinstance(expression, unreal.MaterialExpressionMaterialFunctionCall):
            target = expression.get_editor_property("material_function")
            if target:
                references.append((expression, "material_function", target, "function"))
        if isinstance(expression, unreal.MaterialExpressionTextureBase):
            target = expression.get_editor_property("texture")
            if target:
                references.append((expression, "texture", target, "property"))
        if isinstance(expression, unreal.MaterialExpressionCollectionParameter):
            target = expression.get_editor_property("collection")
            if target:
                references.append((expression, "collection", target, "property"))
        if isinstance(expression, unreal.MaterialExpressionTextureCollection):
            target = expression.get_editor_property("texture_collection")
            if target:
                references.append((expression, "texture_collection", target, "property"))
    return references


def main():
    manifest_path = require_sandbox_project()
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    asset_library = unreal.EditorAssetLibrary
    dependency_options = unreal.AssetRegistryDependencyOptions(
        include_hard_package_references=True,
        include_soft_package_references=True,
        include_game_package_references=True,
        include_editor_only_package_references=True,
    )

    source_packages = set(SOURCE_PACKAGES)
    if len(source_packages) != 24:
        fail("the audited source package list is not exactly 24 unique packages")

    mapping = {}
    manifest = []
    # Validate the entire closure and all target names before making any asset.
    for source_package in sorted(source_packages):
        source_assets = registry.get_assets_by_package_name(source_package) or []
        if len(source_assets) != 1:
            fail("expected exactly one source asset in " + source_package)
        source_data = source_assets[0]
        asset_name = str(source_data.get_editor_property("asset_name"))
        source_object = source_package + "." + asset_name
        destination_object = destination_for(source_package, asset_name)
        if asset_library.does_asset_exist(destination_object):
            fail("destination already exists; no assets copied: " + destination_object)
        source_asset = asset_library.load_asset(source_object)
        if not source_asset:
            fail("source failed to load: " + source_object)
        entry = {
            "source_package": source_package,
            "source_object": source_object,
            "source_class": class_path(source_data),
            "destination_object": destination_object,
            "references_rewired": 0,
        }
        mapping[source_object] = destination_object
        manifest.append(entry)

    if len(manifest) != len(source_packages):
        fail("manifest/package count mismatch")

    for entry in manifest:
        destination_folder = os.path.dirname(entry["destination_object"])
        if not asset_library.does_directory_exist(destination_folder):
            if not asset_library.make_directory(destination_folder):
                fail("could not create sandbox folder: " + destination_folder)
        duplicate = asset_library.duplicate_asset(
            entry["source_object"], entry["destination_object"]
        )
        if not duplicate:
            fail("duplicate failed: " + entry["source_object"])

    duplicate_assets = {}
    for entry in manifest:
        duplicate = asset_library.load_asset(entry["destination_object"])
        if not duplicate:
            fail("copied asset failed to load: " + entry["destination_object"])
        duplicate_assets[entry["source_object"]] = duplicate

    for entry in manifest:
        source_object = entry["source_object"]
        duplicate = duplicate_assets[source_object]
        for expression, property_name, old_target, setter_kind in references_for(duplicate):
            old_path = object_path(old_target)
            if old_path not in mapping:
                if old_path and old_path.startswith(PLUGIN_PREFIX):
                    fail("unmapped PVE graph reference: " + old_path)
                continue
            new_target = asset_library.load_asset(mapping[old_path])
            if not new_target:
                fail("mapped target failed to load: " + mapping[old_path])
            if setter_kind == "function":
                setter = getattr(expression, "set_material_function", None)
                if not callable(setter):
                    fail("function setter unavailable on " + expression.get_class().get_name())
                setter(new_target)
            else:
                expression.set_editor_property(property_name, new_target)
            entry["references_rewired"] += 1

    copied_materials = [
        asset for asset in duplicate_assets.values()
        if isinstance(asset, unreal.Material)
    ]
    if not copied_materials:
        fail("the closure contains no copied master material")
    for material in copied_materials:
        errors = unreal.MaterialEditingLibrary.recompile_material(material) or []
        if errors:
            fail("material compile failed for {}: {}".format(
                object_path(material), "; ".join(str(error) for error in errors)
            ))

    for entry in manifest:
        asset = duplicate_assets[entry["source_object"]]
        if not asset_library.save_loaded_asset(asset):
            fail("save failed: " + entry["destination_object"])

    copied_packages = {package_of(entry["destination_object"]) for entry in manifest}
    pending = list(copied_packages)
    visited = set()
    plugin_dependencies = set()
    while pending:
        package = pending.pop()
        if package in visited:
            continue
        visited.add(package)
        for dependency in registry.get_dependencies(package, dependency_options) or []:
            name = str(dependency)
            if name.startswith(PLUGIN_PREFIX):
                plugin_dependencies.add(name)
            if name.startswith("/Game/") and name not in visited:
                pending.append(name)
    if plugin_dependencies:
        fail("copied closure still references PVE packages: " + ", ".join(sorted(plugin_dependencies)))

    os.makedirs(os.path.dirname(manifest_path), exist_ok=True)
    with open(manifest_path, "w", encoding="utf-8") as output:
        json.dump({"assets": manifest, "count": len(manifest)}, output, indent=2)
    unreal.log(
        "[MegaplantRuntimeMaterialSandbox] COPY_COMPLETE assets={} materials={} rewired={} plugin_dependencies=0".format(
            len(manifest), len(copied_materials),
            sum(entry["references_rewired"] for entry in manifest),
        )
    )


main()
