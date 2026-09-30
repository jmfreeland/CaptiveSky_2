"""Read-only Unreal Editor diagnostic for the island landscape wetness parameter."""

import traceback
import unreal


MATERIAL_PATH = "/Game/Materials/M_Island_Textured_Auto"
PARAMETER_NAME = "Ground Wetness"
INSTANCE_PATH = "/Game/Materials/MI_Island_Landscape"
PROTOTYPE_PATHS = (
    "/Game/Materials/MI_Island_Landscape_WetPrototype",
    "/Game/Materials/MI_Island_Landscape_PuddleScalePrototype",
)


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
    instance = unreal.load_asset(INSTANCE_PATH)
    if instance:
        try:
            add_puddles = unreal.MaterialEditingLibrary.get_material_instance_static_switch_parameter_value(
                instance, "Add Puddles")
            unreal.log("[LandscapeWetness] Landscape MI Add Puddles value: {}".format(add_puddles))
        except Exception as error:
            unreal.log_warning("[LandscapeWetness] Landscape MI switch query failed: {}".format(error))
    for instance_path in PROTOTYPE_PATHS:
        prototype = unreal.load_asset(instance_path)
        if not prototype:
            continue
        try:
            switch_value = unreal.MaterialEditingLibrary.get_material_instance_static_switch_parameter_value(
                prototype, "Add Puddles")
            unreal.log("[LandscapeWetness] Prototype {} Add Puddles readback: {}".format(
                instance_path, switch_value))
            static_parameters = prop(prototype, "static_parameters")
            unreal.log("[LandscapeWetness] Prototype {} static_parameters: {}".format(
                instance_path, static_parameters))
            runtime_parameters = prop(prototype, "static_parameters_runtime")
            unreal.log("[LandscapeWetness] Prototype {} runtime static parameters: {}".format(
                instance_path, runtime_parameters))
            switches = prop(runtime_parameters, "static_switch_parameters", []) if runtime_parameters else []
            for switch in switches or []:
                info = prop(switch, "parameter_info")
                unreal.log("[LandscapeWetness] Prototype switch entry: name={}, association={}, index={}, value={}, override={}, guid={}".format(
                    prop(info, "name"), prop(info, "association"), prop(info, "index"),
                    prop(switch, "value"), prop(switch, "b_override"), prop(switch, "expression_guid")))
            for parameter_name in ("Puddle Size", "Puddle Depth", "Puddle Clarity", "Puddle Constrain"):
                scalar_value = unreal.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(
                    prototype, parameter_name)
                unreal.log("[LandscapeWetness] Prototype {} {} readback: {}".format(
                    instance_path, parameter_name, scalar_value))
        except Exception as error:
            unreal.log_warning("[LandscapeWetness] Prototype query failed for {}: {}".format(
                instance_path, error))
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
        if function.get_name() in ("MF_CreateLayer", "MF_Puddles"):
            try:
                input_names = unreal.MaterialEditingLibrary.get_material_expression_input_names(expression)
                connected_nodes = unreal.MaterialEditingLibrary.get_inputs_for_material_expression(material, expression)
                unreal.log("[LandscapeWetness] {} call input pins: {}".format(function.get_name(), input_names))
                unreal.log("[LandscapeWetness] {} call connected inputs: {}".format(
                    function.get_name(), [label(node) for node in connected_nodes]))
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
        if function.get_name() in ("MF_CreateLayer", "MF_Puddles"):
            for function_expression in function_expressions:
                expression_class = function_expression.get_class().get_name()
                if function.get_name() == "MF_Puddles" and expression_class == "MaterialExpressionScalarParameter":
                    unreal.log("[LandscapeWetness] MF_Puddles scalar: name={}, default={}".format(
                        prop(function_expression, "parameter_name"), prop(function_expression, "default_value")))
                if function.get_name() == "MF_Puddles" and expression_class == "MaterialExpressionStaticSwitchParameter":
                    unreal.log("[LandscapeWetness] MF_Puddles switch: name={}, default={}".format(
                        prop(function_expression, "parameter_name"), prop(function_expression, "default_value")))
                if function.get_name() == "MF_Puddles" and expression_class == "MaterialExpressionTextureSample":
                    texture = prop(function_expression, "texture")
                    unreal.log("[LandscapeWetness] MF_Puddles texture sample: texture={}, sampler_type={}".format(
                        texture.get_path_name() if texture else "<none>",
                        prop(function_expression, "sampler_type")))
                if function.get_name() == "MF_Puddles" and expression_class in (
                        "MaterialExpressionSetMaterialAttributes", "MaterialExpressionGetMaterialAttributes"):
                    try:
                        unreal.log("[LandscapeWetness] MF_Puddles attribute node {} inputs={} outputs={}".format(
                            label(function_expression),
                            unreal.MaterialEditingLibrary.get_material_expression_input_names(function_expression),
                            unreal.MaterialEditingLibrary.get_material_expression_output_names(function_expression)))
                    except Exception as error:
                        unreal.log_warning("[LandscapeWetness] Attribute pin query failed: {}".format(error))
                try:
                    inputs = unreal.MaterialEditingLibrary.get_inputs_for_material_function_expression(function, function_expression)
                except Exception:
                    inputs = []
                if inputs:
                    unreal.log("[LandscapeWetness] {} graph: {} <- {}".format(
                        function.get_name(), label(function_expression), [label(node) for node in inputs]))
                if function_expression.get_class().get_name() == "MaterialExpressionFunctionInput":
                    unreal.log("[LandscapeWetness] {} input: name={}, description={}, preview={}".format(
                        function.get_name(),
                        prop(function_expression, "input_name"), prop(function_expression, "description"),
                        prop(function_expression, "preview_value")))

    unreal.log("[LandscapeWetness] Read-only inspection complete; no assets were modified or saved.")


try:
    main()
except Exception:
    unreal.log_error("[LandscapeWetness] Inspection failed:\n{}".format(traceback.format_exc()))
