"""Read-only audit of plant LOD slots and nested material function references.

Run in UE 5.8.3 with -ExecutePythonScript=<absolute script path> -unattended
-NoZen -DDC-ForceMemoryCache -NullRHI -abslog=<absolute log path>.
No map load, saves, recompiles, gameplay, or external calls. JSON is emitted to
the editor log. A clean reference graph is NOT proof of shader/visual health.
"""

import json
import traceback
import unreal


MESH_PATHS = (
    "/Game/Plants/Meshes/Festuca_gautieri_LD",
    "/Game/Plants/Meshes/Phalaris_arundinacea_LD",
    "/Game/Plants/Meshes/Typha_latifolia_LD",
    "/Game/Plants/Meshes/Rhododendron__Everestianum__HD",
    "/Game/PN_interactiveSpruceForest/Meshes/half/high/spruce_half_01",
)


def inspect_graph(material, cache):
    path = material.get_path_name()
    if path in cache:
        return cache[path]
    report = {"path": path, "functions": [], "missing_function_calls": [], "errors": []}
    cache[path] = report
    try:
        library = unreal.MaterialEditingLibrary
        instance_mesh_usage = unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES
        if isinstance(material, (unreal.Material, unreal.MaterialInstance)):
            report["usage"] = {
                "instanced_static_meshes": bool(library.has_material_usage(material, instance_mesh_usage))
            }
        if isinstance(material, unreal.MaterialInstance):
            if isinstance(material, unreal.MaterialInstanceConstant):
                report["usage"]["instanced_static_mesh_override_exists"] = bool(
                    library.has_material_usage_override(material, instance_mesh_usage)
                )
            report["static_switches"] = {
                str(name): library.get_material_instance_static_switch_parameter_value(material, name)
                for name in library.get_static_switch_parameter_names(material)
            }
            parent = material.get_editor_property("parent")
            report["parent"] = parent.get_path_name() if parent else None
            if parent:
                inspect_graph(parent, cache)
            else:
                report["errors"].append("Material instance has no parent")
            return report
        if isinstance(material, unreal.Material):
            report["static_switches"] = {
                str(name): library.get_material_default_static_switch_parameter_value(material, name)
                for name in library.get_static_switch_parameter_names(material)
            }
            expressions = library.get_material_expressions(material)
        elif isinstance(material, unreal.MaterialFunction):
            expressions = library.get_material_function_expressions(material)
        else:
            report["errors"].append("Unsupported graph type: " + material.get_class().get_name())
            return report
        report["expression_count"] = len(expressions)
        for expression in expressions:
            if not isinstance(expression, unreal.MaterialExpressionMaterialFunctionCall):
                continue
            function = expression.get_editor_property("material_function")
            if function:
                report["functions"].append(function.get_path_name())
                inspect_graph(function, cache)
            else:
                report["missing_function_calls"].append(expression.get_path_name())
    except Exception as error:
        report["errors"].append(str(error))
    return report


def main():
    report = {"meshes": [], "graphs": {}, "limitations": [
        "Reference audit only: does not compile shaders or prove connected graph reachability.",
        "LOD section use is recorded, but camera-distance selection is not exercised.",
        "Material usage and instance-override flags are reported read-only; no material is recompiled or saved.",
    ]}
    subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    for path in MESH_PATHS:
        entry = {"path": path, "slots": [], "lods": [], "errors": []}
        report["meshes"].append(entry)
        mesh = unreal.load_asset(path)
        if not mesh:
            entry["errors"].append("Mesh missing")
            continue
        for index, slot in enumerate(mesh.get_editor_property("static_materials")):
            material = slot.get_editor_property("material_interface")
            entry["slots"].append({"index": index, "name": str(slot.get_editor_property("material_slot_name")),
                                   "material": material.get_path_name() if material else None})
            if material:
                inspect_graph(material, report["graphs"])
            else:
                entry["errors"].append("Missing material at slot {}".format(index))
        try:
            for lod in range(subsystem.get_lod_count(mesh)):
                sections = [subsystem.get_lod_material_slot(mesh, lod, section)
                            for section in range(mesh.get_num_sections(lod))]
                entry["lods"].append({"index": lod, "material_slots": sections})
        except Exception as error:
            entry["errors"].append("LOD inspection unavailable: " + str(error))
    issues = sum(len(graph["missing_function_calls"]) + len(graph["errors"]) for graph in report["graphs"].values())
    issues += sum(len(mesh["errors"]) for mesh in report["meshes"])
    report["reference_issue_count"] = issues
    report["materials_missing_instanced_static_mesh_usage"] = [
        path for path, graph in sorted(report["graphs"].items())
        if graph.get("usage", {}).get("instanced_static_meshes") is False
    ]
    unreal.log("[PlantAudit] REPORT " + json.dumps(report, sort_keys=True))
    unreal.log("[PlantAudit] COMPLETE reference_issues={} materials_missing_instanced_static_mesh_usage={}".format(
        issues, len(report["materials_missing_instanced_static_mesh_usage"])))


try:
    main()
except Exception:
    unreal.log_error("[PlantAudit] FAILED " + traceback.format_exc())
    raise
