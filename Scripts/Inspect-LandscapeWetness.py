"""Read-only Unreal Editor diagnostic for the island landscape wetness parameter."""

import traceback
import unreal


MATERIAL_PATH = "/Game/Materials/M_Island_Textured_Auto"
PARAMETER_NAME = "Ground Wetness"


def label(expression):
    try:
        return "{} [{}]".format(expression.get_name(), expression.get_class().get_name())
    except Exception:
        return str(expression)


def prop(obj, name, default=None):
    try:
        return obj.get_editor_property(name)
    except Exception:
        return default


def main():
    material = unreal.load_asset(MATERIAL_PATH)
    if not material:
        raise RuntimeError("Could not load " + MATERIAL_PATH)

    unreal.log("[LandscapeWetness] Material: {}".format(material.get_path_name()))
    expressions = unreal.MaterialEditingLibrary.get_material_expressions(material)
    targets = []
    for expression in expressions:
        if expression.get_class().get_name() == "MaterialExpressionScalarParameter":
            if str(prop(expression, "parameter_name")) == PARAMETER_NAME:
                targets.append(expression)
                unreal.log("[LandscapeWetness] Parameter expression: {}".format(label(expression)))

    if not targets:
        unreal.log_warning("[LandscapeWetness] No Ground Wetness scalar expression found in parent material")

    for expression in expressions:
        try:
            inputs = unreal.MaterialEditingLibrary.get_inputs_for_material_expression(material, expression)
        except Exception as error:
            unreal.log_warning("[LandscapeWetness] Input query failed for {}: {}".format(label(expression), error))
            continue
        for input_expression in inputs:
            if input_expression in targets:
                unreal.log("[LandscapeWetness] Direct edge: {} -> {}".format(label(input_expression), label(expression)))

    for expression in expressions:
        if expression.get_class().get_name() != "MaterialExpressionMaterialFunctionCall":
            continue
        function = prop(expression, "material_function")
        if not function:
            continue
        unreal.log("[LandscapeWetness] Function call: {} -> {}".format(label(expression), function.get_path_name()))
        if function.get_name() == "MF_CreateLayer":
            try:
                input_names = unreal.MaterialEditingLibrary.get_material_expression_input_names(expression)
                connected_nodes = unreal.MaterialEditingLibrary.get_inputs_for_material_expression(material, expression)
                unreal.log("[LandscapeWetness] MF_CreateLayer call input pins: {}".format(input_names))
                unreal.log("[LandscapeWetness] MF_CreateLayer call connected inputs: {}".format([label(node) for node in connected_nodes]))
            except Exception as error:
                unreal.log_warning("[LandscapeWetness] Call input details unavailable: {}".format(error))
        try:
            function_expressions = unreal.MaterialEditingLibrary.get_material_function_expressions(function)
        except Exception as error:
            unreal.log_warning("[LandscapeWetness] Function enumeration failed: {}".format(error))
            continue
        function_targets = []
        for function_expression in function_expressions:
            if function_expression.get_class().get_name() == "MaterialExpressionScalarParameter":
                if str(prop(function_expression, "parameter_name")) == PARAMETER_NAME:
                    function_targets.append(function_expression)
                    unreal.log("[LandscapeWetness] Function parameter: {}".format(label(function_expression)))
        for function_expression in function_expressions:
            try:
                inputs = unreal.MaterialEditingLibrary.get_inputs_for_material_function_expression(function, function_expression)
            except Exception as error:
                unreal.log_warning("[LandscapeWetness] Function input query failed for {}: {}".format(label(function_expression), error))
                continue
            for input_expression in inputs:
                if input_expression in function_targets:
                    unreal.log("[LandscapeWetness] Function edge: {} -> {}".format(label(input_expression), label(function_expression)))
            if function_expression.get_class().get_name() == "MaterialExpressionFunctionOutput":
                unreal.log("[LandscapeWetness] Function output: {}".format(label(function_expression)))
                unreal.log("[LandscapeWetness] Output properties: name={}, description={}".format(
                    prop(function_expression, "output_name"), prop(function_expression, "description")))
        if function.get_name() == "MF_CreateLayer":
            for function_expression in function_expressions:
                try:
                    inputs = unreal.MaterialEditingLibrary.get_inputs_for_material_function_expression(function, function_expression)
                except Exception:
                    inputs = []
                if inputs:
                    unreal.log("[LandscapeWetness] MF_CreateLayer graph: {} <- {}".format(
                        label(function_expression), [label(node) for node in inputs]))
                if function_expression.get_class().get_name() == "MaterialExpressionFunctionInput":
                    unreal.log("[LandscapeWetness] MF_CreateLayer input: name={}, description={}, preview={}".format(
                        prop(function_expression, "input_name"), prop(function_expression, "description"),
                        prop(function_expression, "preview_value")))

    unreal.log("[LandscapeWetness] Read-only inspection complete; no assets were modified or saved.")


try:
    main()
except Exception:
    unreal.log_error("[LandscapeWetness] Inspection failed:\n{}".format(traceback.format_exc()))
