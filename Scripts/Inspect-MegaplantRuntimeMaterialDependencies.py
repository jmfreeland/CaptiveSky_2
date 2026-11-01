"""Read-only audit of runtime Megaplant material parents and dependencies.

Run in UE 5.8.3 with the ProceduralVegetationEditor plugin enabled so its
sample assets are mounted, using -ExecutePythonScript=<absolute path>.
The script loads no level, saves no assets, and makes no changes. It reports
every material slot on the A/B tree variants used by IslandWeather, their
material-instance parent chains, and recursively referenced packages. Plugin
content in a runtime material dependency closure is a shipping risk when that
plugin is editor-only or unavailable in the game target.
"""

import json
import traceback
import unreal


TREE_ASSETS = (
    "/Game/Megaplant_Library/Tree_Norway_Maple/Tree_Norway_Maple_Forest_01/Tree_Norway_Maple_Forest_01_A",
    "/Game/Megaplant_Library/Tree_Norway_Maple/Tree_Norway_Maple_Forest_01/Tree_Norway_Maple_Forest_01_B",
    "/Game/Megaplant_Library/Tree_European_Aspen/Tree_European_Aspen_01/Tree_European_Aspen_01_A",
    "/Game/Megaplant_Library/Tree_European_Aspen/Tree_European_Aspen_01/Tree_European_Aspen_01_B",
    "/Game/Megaplant_Library/Tree_European_Beech/Tree_European_Beech_01/Tree_European_Beech_01_A",
    "/Game/Megaplant_Library/Tree_European_Beech/Tree_European_Beech_01/Tree_European_Beech_01_B",
    "/Game/Megaplant_Library/Tree_Black_Alder/Tree_Black_Alder_01/Tree_Black_Alder_01_A",
    "/Game/Megaplant_Library/Tree_Black_Alder/Tree_Black_Alder_01/Tree_Black_Alder_01_B",
)

PLUGIN_PREFIXES = (
    "/ProceduralVegetationEditor/",
    "/ProceduralVegetation/",
)


def object_path(asset):
    return asset.get_path_name() if asset else None


def package_name(path):
    return path.split(".", 1)[0]


def material_parent_chain(material):
    chain = []
    seen = set()
    current = material
    while current:
        path = object_path(current)
        if path in seen:
            chain.append({"path": path, "error": "parent cycle"})
            break
        seen.add(path)
        entry = {"path": path, "class": current.get_class().get_name()}
        chain.append(entry)
        if isinstance(current, unreal.MaterialInstance):
            current = current.get_editor_property("parent")
        else:
            break
    return chain


def dependency_closure(asset_registry, roots):
    options = unreal.AssetRegistryDependencyOptions(
        include_hard_package_references=True,
        include_soft_package_references=True,
        include_game_package_references=True,
        include_editor_only_package_references=True,
    )
    visited = set()
    pending = [package_name(path) for path in roots if path]
    while pending:
        package = pending.pop()
        if package in visited:
            continue
        visited.add(package)
        dependencies = asset_registry.get_dependencies(package, options) or []
        pending.extend(str(dependency) for dependency in dependencies if str(dependency) not in visited)
    return sorted(visited)


def inspect_tree(path, asset_registry):
    result = {"mesh": path, "material_slots": [], "errors": []}
    mesh = unreal.EditorAssetLibrary.load_asset(path)
    if not mesh:
        result["errors"].append("mesh failed to load")
        return result
    if not isinstance(mesh, unreal.SkeletalMesh):
        result["errors"].append("asset is not a SkeletalMesh")
        return result

    material_roots = []
    for slot_index, slot in enumerate(mesh.get_editor_property("materials")):
        material = slot.get_editor_property("material_interface")
        chain = material_parent_chain(material)
        slot_result = {"slot": slot_index, "parent_chain": chain}
        result["material_slots"].append(slot_result)
        material_roots.extend(entry.get("path") for entry in chain if entry.get("path"))

    packages = dependency_closure(asset_registry, material_roots)
    result["dependency_packages"] = packages
    result["plugin_dependencies"] = [
        package for package in packages
        if any(package.startswith(prefix) for prefix in PLUGIN_PREFIXES)
    ]
    return result


def main():
    asset_registry = unreal.AssetRegistryHelpers.get_asset_registry()
    report = {
        "read_only": True,
        "tree_count": len(TREE_ASSETS),
        "trees": [inspect_tree(path, asset_registry) for path in TREE_ASSETS],
    }
    plugin_packages = sorted({
        package
        for tree in report["trees"]
        for package in tree.get("plugin_dependencies", [])
    })
    report["unique_plugin_dependencies"] = plugin_packages
    report["unique_plugin_dependency_count"] = len(plugin_packages)
    unreal.log("[MegaplantRuntimeMaterialAudit] REPORT " + json.dumps(report, sort_keys=True))
    unreal.log(
        "[MegaplantRuntimeMaterialAudit] COMPLETE trees={} plugin_dependencies={}".format(
            len(report["trees"]), len(plugin_packages)
        )
    )


try:
    main()
except Exception:
    unreal.log_error("[MegaplantRuntimeMaterialAudit] FAILED " + traceback.format_exc())
    raise
