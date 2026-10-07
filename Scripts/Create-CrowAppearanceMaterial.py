"""Create a separate, adjustable dark-feather material for CaptiveSky's Raven.

Run with UE 5.8.3:
  UnrealEditor-Cmd.exe <project> -ExecutePythonScript=<this file> -unattended -NoZen -abslog=<log>

This duplicates the optional Fab Crow material and scales only its Base Color. The source asset is
never edited. The output is local Content (gitignored); the runtime Raven prefers it when present
and otherwise keeps using the original material.
"""

import traceback
import unreal


SOURCE = "/Game/AnimalVarietyPack/Crow/Materials/M_Crow"
DESTINATION = "/Game/AnimalVarietyPack/Crow/Materials/M_Crow_CaptiveSky"
BRIGHTNESS_PARAMETER = "FeatherAlbedoScale"
DEFAULT_BRIGHTNESS = 0.025


def main():
    assets = unreal.EditorAssetLibrary
    library = unreal.MaterialEditingLibrary
    source = unreal.load_asset(SOURCE)
    if not source or not isinstance(source, unreal.Material):
        raise RuntimeError("Missing or unsupported source material " + SOURCE)

    b_existing_variant = assets.does_asset_exist(DESTINATION)
    variant = assets.load_asset(DESTINATION) if b_existing_variant else None
    if not variant:
        variant = assets.duplicate_asset(SOURCE, DESTINATION)
    if not variant or not isinstance(variant, unreal.Material):
        raise RuntimeError("Could not load or duplicate " + SOURCE)

    base_color = unreal.MaterialProperty.MP_BASE_COLOR
    expressions = list(library.get_material_expressions(variant))
    scale = next((expression for expression in expressions
                  if isinstance(expression, unreal.MaterialExpressionScalarParameter)
                  and str(expression.get_editor_property("parameter_name")) == BRIGHTNESS_PARAMETER), None)
    multiply = next((expression for expression in expressions
                     if isinstance(expression, unreal.MaterialExpressionMultiply)
                     and scale in library.get_inputs_for_material_expression(variant, expression)), None) if scale else None
    if b_existing_variant and (not scale or not multiply):
        raise RuntimeError("Refusing to alter an existing material without the expected CaptiveSky graph")
    if not scale:
        source_node = library.get_material_property_input_node(variant, base_color)
        if not source_node:
            raise RuntimeError("Source material has no Base Color connection")
        source_pin = library.get_material_property_input_node_output_name(variant, base_color)
        scale = library.create_material_expression(
            variant, unreal.MaterialExpressionScalarParameter, -500, 300)
        multiply = library.create_material_expression(
            variant, unreal.MaterialExpressionMultiply, -180, 0)
        if not scale or not multiply:
            raise RuntimeError("Could not create the feather brightness graph nodes")
        scale.set_editor_property("parameter_name", BRIGHTNESS_PARAMETER)
        scale.set_editor_property("group", "CaptiveSky Raven")
        if not library.connect_material_expressions(source_node, source_pin, multiply, "A"):
            raise RuntimeError("Could not connect the authored Base Color")
        if not library.connect_material_expressions(scale, "", multiply, "B"):
            raise RuntimeError("Could not connect the feather brightness parameter")
        if not library.connect_material_property(multiply, "", base_color):
            raise RuntimeError("Could not connect the adjusted Base Color output")
    scale.set_editor_property("default_value", DEFAULT_BRIGHTNESS)

    errors = library.recompile_material(variant)
    if errors:
        raise RuntimeError("Material compile errors: " + str(errors))
    if not assets.save_asset(DESTINATION):
        raise RuntimeError("Could not save " + DESTINATION)

    unreal.log("[CrowAppearance] Created {} from {}; {}={}".format(
        DESTINATION, SOURCE, BRIGHTNESS_PARAMETER, DEFAULT_BRIGHTNESS))


try:
    main()
except Exception:
    unreal.log_error("[CrowAppearance] FAILED " + traceback.format_exc())
    raise
