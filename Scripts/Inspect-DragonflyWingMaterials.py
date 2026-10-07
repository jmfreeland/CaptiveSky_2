import unreal

MATERIALS = (
    "/Engine/EngineDebugMaterials/M_SimpleUnlitTranslucent",
    "/Engine/EngineDebugMaterials/M_SimpleTranslucent",
    "/Engine/EngineDebugMaterials/VertexColorMaterial",
)

for asset_path in MATERIALS:
    material = unreal.load_asset(asset_path)
    print("DRAGONFLY_MATERIAL", asset_path, "loaded=", bool(material))
    if not material:
        continue

    for prop_name in ("blend_mode", "shading_model"):
        try:
            value = material.get_editor_property(prop_name)
            print("DRAGONFLY_MATERIAL_PROPERTY", asset_path, prop_name, value)
        except Exception as exc:
            print("DRAGONFLY_MATERIAL_PROPERTY_ERROR", asset_path, prop_name, str(exc))

    try:
        for expression in unreal.MaterialEditingLibrary.get_material_expressions(material):
            fields = []
            for field_name in ("parameter_name", "const_value", "default_value", "rgb", "opacity"):
                try:
                    fields.append(f"{field_name}={expression.get_editor_property(field_name)}")
                except Exception:
                    pass
            print("DRAGONFLY_MATERIAL_EXPRESSION", expression.get_class().get_name(), "; ".join(fields))
        for property_name in ("MP_BASE_COLOR", "MP_EMISSIVE_COLOR", "MP_OPACITY"):
            try:
                prop = getattr(unreal.MaterialProperty, property_name)
                node = unreal.MaterialEditingLibrary.get_material_property_input_node(material, prop)
                print("DRAGONFLY_MATERIAL_OUTPUT", asset_path, property_name,
                      node.get_class().get_name() if node else "None",
                      unreal.MaterialEditingLibrary.get_material_property_input_node_output_name(material, prop))
            except Exception as exc:
                print("DRAGONFLY_MATERIAL_OUTPUT_ERROR", asset_path, property_name, str(exc))
    except Exception as exc:
        print("DRAGONFLY_MATERIAL_GRAPH_ERROR", asset_path, str(exc))
