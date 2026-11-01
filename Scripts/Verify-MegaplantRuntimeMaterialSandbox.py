"""Compile the copied Megaplant master material after the PVE plugin is disabled."""

import json
import os
import unreal


MANIFEST_RELATIVE_PATH = os.path.join("MegaplantRuntimeMaterialSandbox", "copy-manifest.json")
PLUGIN_PREFIX = "/ProceduralVegetationEditor/"


def fail(message):
    raise RuntimeError("[MegaplantRuntimeMaterialSandbox] " + message)


def require_sandbox_manifest():
    project_dir = os.path.realpath(unreal.Paths.project_dir())
    project_name = os.path.basename(os.path.normpath(project_dir))
    project_file = os.path.join(project_dir, "MegaplantRuntimeMaterialSandbox.uproject")
    if project_name.casefold() != "megaplantruntimematerialsandbox" or not os.path.isfile(project_file):
        fail("refusing to verify assets outside the dedicated MegaplantRuntimeMaterialSandbox project")
    saved_dir = os.path.realpath(unreal.Paths.project_saved_dir())
    manifest_path = os.path.realpath(os.path.join(saved_dir, MANIFEST_RELATIVE_PATH))
    if os.path.commonpath((saved_dir, manifest_path)) != saved_dir:
        fail("copy manifest path escaped the sandbox Saved directory")
    return manifest_path


manifest_path = require_sandbox_manifest()
with open(manifest_path, "r", encoding="utf-8") as source:
    manifest = json.load(source)

if manifest.get("count") != len(manifest.get("assets", [])) or not manifest.get("assets"):
    fail("copy manifest is empty or inconsistent")

assets = []
for entry in manifest["assets"]:
    asset = unreal.EditorAssetLibrary.load_asset(entry["destination_object"])
    if not asset:
        fail("copied asset failed to load with PVE disabled: " + entry["destination_object"])
    assets.append(asset)

materials = [asset for asset in assets if isinstance(asset, unreal.Material)]
if not materials:
    fail("manifest contains no project-owned master material")

for material in materials:
    errors = unreal.MaterialEditingLibrary.recompile_material(material) or []
    if errors:
        fail("material compile failed for {}: {}".format(
            material.get_path_name(), "; ".join(str(error) for error in errors)
        ))

registry = unreal.AssetRegistryHelpers.get_asset_registry()
options = unreal.AssetRegistryDependencyOptions(
    include_hard_package_references=True,
    include_soft_package_references=True,
    include_game_package_references=True,
    include_editor_only_package_references=True,
)
pending = [entry["destination_object"].split(".", 1)[0] for entry in manifest["assets"]]
visited = set()
plugin_dependencies = set()
while pending:
    package = pending.pop()
    if package in visited:
        continue
    visited.add(package)
    for dependency in registry.get_dependencies(package, options) or []:
        name = str(dependency)
        if name.startswith(PLUGIN_PREFIX):
            plugin_dependencies.add(name)
        if name.startswith("/Game/") and name not in visited:
            pending.append(name)

if plugin_dependencies:
    fail("runtime copy references plugin packages: " + ", ".join(sorted(plugin_dependencies)))

unreal.log(
    "[MegaplantRuntimeMaterialSandbox] NO_PLUGIN_VERIFY_COMPLETE assets={} materials={} plugin_dependencies=0".format(
        len(assets), len(materials)
    )
)
