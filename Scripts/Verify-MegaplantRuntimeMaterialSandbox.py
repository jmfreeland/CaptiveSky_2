"""Compile the copied Megaplant master material after the PVE plugin is disabled."""

import json
import unreal


MANIFEST_PATH = "Saved/MegaplantRuntimeMaterialSandbox/copy-manifest.json"
PLUGIN_PREFIX = "/ProceduralVegetationEditor/"


def fail(message):
    raise RuntimeError("[MegaplantRuntimeMaterialSandbox] " + message)


with open(MANIFEST_PATH, "r", encoding="utf-8") as source:
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
