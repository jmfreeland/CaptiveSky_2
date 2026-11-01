"""Read-only audit of runtime Megaplant material parents and dependencies.

Run in UE 5.8.3 with the ProceduralVegetationEditor plugin enabled so its
sample assets are mounted, using -ExecutePythonScript=<absolute path>.
The script loads no level, saves no assets, and makes no changes. It reports
every material slot on the A/B tree variants used by IslandWeather, their
material-instance parent chains, and recursively referenced packages. Plugin
content in a runtime material dependency closure is a shipping risk when that
plugin is editor-only or unavailable in the game target.
The report includes instance texture/scalar/vector/static-switch overrides,
nested material-function calls, and direct texture/collection references so a
project-owned runtime material can be designed against the installed textures
without editing any assets.
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
    "/Game/Megaplant_Library/Tree_Common_Hazel/Tree_Common_Hazel_01/Tree_Common_Hazel_01_A",
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


def parameter_overrides(instance, property_name, value_property, render_path=False):
    values = instance.get_editor_property(property_name) or []
    result = []
    for value in values:
        info = value.get_editor_property("parameter_info")
        parameter_name = str(info.get_editor_property("name"))
        parameter_value = value.get_editor_property(value_property)
        if render_path and parameter_value:
            parameter_value = object_path(parameter_value)
        else:
            parameter_value = str(parameter_value)
        result.append({"name": parameter_name, "value": parameter_value})
    return result


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


def material_function_graph(roots):
    """Inspect Material/MaterialFunction calls without compiling or editing them."""
    pending = [path for path in roots if path]
    visited = set()
    graph = []
    while pending:
        path = pending.pop()
        if path in visited:
            continue
        visited.add(path)
        asset = unreal.EditorAssetLibrary.load_asset(path)
        if isinstance(asset, unreal.Material):
            expressions = unreal.MaterialEditingLibrary.get_material_expressions(asset)
        elif isinstance(asset, unreal.MaterialFunction):
            expressions = unreal.MaterialEditingLibrary.get_material_function_expressions(asset)
        else:
            continue

        calls = []
        asset_references = []
        for expression in expressions:
            if isinstance(expression, unreal.MaterialExpressionMaterialFunctionCall):
                function = expression.get_editor_property("material_function")
                function_path = object_path(function)
                calls.append({
                    "node": expression.get_name(),
                    "function": function_path,
                    "setter_available": callable(getattr(expression, "set_material_function", None)),
                })
                if function_path and function_path not in visited:
                    pending.append(function_path)

            reference_properties = []
            if isinstance(expression, unreal.MaterialExpressionTextureBase):
                reference_properties.append(("texture", "texture"))
            if isinstance(expression, unreal.MaterialExpressionCollectionParameter):
                reference_properties.append(("material_parameter_collection", "collection"))
            if isinstance(expression, unreal.MaterialExpressionTextureCollection):
                reference_properties.append(("texture_collection", "texture_collection"))
            for reference_kind, property_name in reference_properties:
                referenced_asset = expression.get_editor_property(property_name)
                asset_references.append({
                    "graph_asset": path,
                    "node": expression.get_name(),
                    "node_class": expression.get_class().get_name(),
                    "kind": reference_kind,
                    "asset": object_path(referenced_asset),
                })

        graph.append({
            "asset": path,
            "class": asset.get_class().get_name(),
            "function_calls": calls,
            "asset_references": asset_references,
        })
    return sorted(graph, key=lambda entry: entry["asset"])


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
        if isinstance(material, unreal.MaterialInstance):
            slot_result["overrides"] = {
                "texture": parameter_overrides(material, "texture_parameter_values", "parameter_value", True),
                "scalar": parameter_overrides(material, "scalar_parameter_values", "parameter_value"),
                "vector": parameter_overrides(material, "vector_parameter_values", "parameter_value"),
            }
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
    material_roots = sorted({
        entry.get("path")
        for tree in report["trees"]
        for slot in tree.get("material_slots", [])
        for entry in slot.get("parent_chain", [])
        if entry.get("path")
    })
    report["material_function_graph"] = material_function_graph(material_roots)
    report["unique_plugin_dependencies"] = plugin_packages
    report["unique_plugin_dependency_count"] = len(plugin_packages)
    graph_calls = [
        call
        for asset in report["material_function_graph"]
        for call in asset["function_calls"]
    ]
    graph_asset_references = [
        reference
        for asset in report["material_function_graph"]
        for reference in asset["asset_references"]
    ]
    unresolved_calls = [call for call in graph_calls if not call["function"]]
    missing_setters = [call for call in graph_calls if not call["setter_available"]]
    report["material_graph_asset_references"] = graph_asset_references
    report["material_graph_asset_reference_count"] = len(graph_asset_references)
    referenced_plugin_assets = sorted({
        reference["asset"]
        for reference in graph_asset_references
        if reference["asset"] and any(reference["asset"].startswith(prefix) for prefix in PLUGIN_PREFIXES)
    })
    report["material_graph_plugin_asset_references"] = referenced_plugin_assets
    unreal.log("[MegaplantRuntimeMaterialAudit] REPORT " + json.dumps(report, sort_keys=True))
    unreal.log(
        "[MegaplantRuntimeMaterialAudit] COMPLETE trees={} plugin_dependencies={} graph_assets={} function_calls={} unresolved_calls={} missing_setters={} direct_references={} plugin_direct_assets={}".format(
            len(report["trees"]), len(plugin_packages), len(report["material_function_graph"]),
            len(graph_calls), len(unresolved_calls), len(missing_setters),
            len(graph_asset_references), len(referenced_plugin_assets)
        )
    )


try:
    main()
except Exception:
    unreal.log_error("[MegaplantRuntimeMaterialAudit] FAILED " + traceback.format_exc())
    raise
