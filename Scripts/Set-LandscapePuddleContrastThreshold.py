"""Retune only the ignored capture-only puddle contrast material."""

import traceback
import unreal


MATERIAL_PATH = "/Game/Materials/M_Island_PuddleNoiseContrastDebug"
PARAMETER_NAME = "Noise Threshold"
THRESHOLD = 0.55


def update():
    material = unreal.load_asset(MATERIAL_PATH)
    if not material:
        raise RuntimeError("Diagnostic material does not exist: " + MATERIAL_PATH)

    matches = []
    for expression in unreal.MaterialEditingLibrary.get_material_expressions(material):
        if expression.get_class().get_name() != "MaterialExpressionScalarParameter":
            continue
        if expression.get_editor_property("parameter_name") == PARAMETER_NAME:
            matches.append(expression)
    if len(matches) != 1:
        raise RuntimeError("Expected one {} parameter; found {}".format(PARAMETER_NAME, len(matches)))

    matches[0].set_editor_property("default_value", THRESHOLD)
    errors = unreal.MaterialEditingLibrary.recompile_material(material)
    if errors:
        raise RuntimeError("Material compile errors: " + " | ".join(str(error) for error in errors))
    if not unreal.EditorAssetLibrary.save_asset(MATERIAL_PATH):
        raise RuntimeError("Could not save " + MATERIAL_PATH)
    unreal.log("[PuddleContrastDebug] Retuned {} to {} on the capture-only asset".format(
        PARAMETER_NAME, THRESHOLD))


try:
    update()
except Exception:
    unreal.log_error("[PuddleContrastDebug] Retune failed:\n{}".format(traceback.format_exc()))
