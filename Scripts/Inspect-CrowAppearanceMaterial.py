"""Read-only audit of the original and CaptiveSky Crow material graphs.

Run with UE 5.8.3:
  UnrealEditor-Cmd.exe <project> -ExecutePythonScript=<this file> -unattended -NoZen
"""

import json
import unreal


MATERIALS = (
    "/Game/AnimalVarietyPack/Crow/Materials/M_Crow",
    "/Game/AnimalVarietyPack/Crow/Materials/M_Crow_CaptiveSky",
)


def inspect(path):
    material = unreal.load_asset(path)
    if not material or not isinstance(material, unreal.Material):
        return {"path": path, "error": "missing or not a UMaterial"}
    library = unreal.MaterialEditingLibrary
    result = {
        "path": material.get_path_name(),
        "uses_material_attributes": material.get_editor_property("use_material_attributes"),
        "shading_model": str(material.get_editor_property("shading_model")),
        "blend_mode": str(material.get_editor_property("blend_mode")),
        "expressions": [],
        "base_color_input": None,
        "emissive_input": None,
        "material_attributes_input": None,
    }
    for prop, key in (
        (unreal.MaterialProperty.MP_BASE_COLOR, "base_color_input"),
        (unreal.MaterialProperty.MP_EMISSIVE_COLOR, "emissive_input"),
        (unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES, "material_attributes_input"),
    ):
        try:
            node = library.get_material_property_input_node(material, prop)
            output = library.get_material_property_input_node_output_name(material, prop) if node else None
            result[key] = {"class": node.get_class().get_name(), "output": str(output)} if node else None
        except Exception as error:
            result[key] = {"error": str(error)}
    for expression in library.get_material_expressions(material):
        item = {"class": expression.get_class().get_name(), "name": expression.get_name()}
        if isinstance(expression, unreal.MaterialExpressionMultiply):
            item["input_pins"] = list(library.get_material_expression_input_names(expression))
            item["connected_inputs"] = [
                {"class": node.get_class().get_name(), "name": node.get_name()}
                for node in library.get_inputs_for_material_expression(material, expression) if node
            ]
        if isinstance(expression, unreal.MaterialExpressionScalarParameter):
            item["parameter"] = str(expression.get_editor_property("parameter_name"))
            item["default"] = expression.get_editor_property("default_value")
        elif isinstance(expression, unreal.MaterialExpressionTextureSample):
            texture = expression.get_editor_property("texture")
            item["texture"] = texture.get_path_name() if texture else None
        result["expressions"].append(item)
    return result


report = [inspect(path) for path in MATERIALS]
unreal.log("[CrowAppearanceAudit] " + json.dumps(report, sort_keys=True))

for mesh_path in (
    "/Game/AnimalVarietyPack/Crow/Meshes/SK_Crow",
    "/Game/AnimalVarietyPack/Crow/Meshes/SK_Crow_CaptiveSky",
):
    crow_mesh = unreal.load_asset(mesh_path)
    try:
        slots = [
            {
                "name": str(slot.get_editor_property("material_slot_name")),
                "material": (slot.get_editor_property("material_interface").get_path_name()
                             if slot.get_editor_property("material_interface") else None),
            }
            for slot in crow_mesh.get_editor_property("materials")
        ] if crow_mesh else None
    except Exception as error:
        slots = {"error": str(error)}
    unreal.log("[CrowAppearanceAudit] mesh={} slots={}".format(
        mesh_path, json.dumps(slots, sort_keys=True)))
