"""Create an isolated landscape instance with larger, clearer puddle patches."""

import traceback
import unreal


SOURCE = "/Game/Materials/MI_Island_Landscape"
DESTINATION = "/Game/Materials/MI_Island_Landscape_PuddleScalePrototype"
SWITCH_NAME = "Add Puddles"
SCALAR_OVERRIDES = {
    "Puddle Size": 500.0,
    "Puddle Depth": 3.0,
    "Puddle Clarity": 0.5,
    "Puddle Constrain": 1.0,
}


def main():
    if unreal.EditorAssetLibrary.does_asset_exist(DESTINATION):
        raise RuntimeError("Refusing to overwrite existing prototype: " + DESTINATION)

    source = unreal.load_asset(SOURCE)
    if not source:
        raise RuntimeError("Could not load source material instance: " + SOURCE)
    if unreal.MaterialEditingLibrary.get_material_instance_static_switch_parameter_value(source, SWITCH_NAME):
        raise RuntimeError("Expected the authored Add Puddles switch to remain disabled")

    prototype = unreal.EditorAssetLibrary.duplicate_asset(SOURCE, DESTINATION)
    if not prototype:
        raise RuntimeError("Could not duplicate source instance")

    unreal.MaterialEditingLibrary.set_material_instance_static_switch_parameter_value(
        prototype, SWITCH_NAME, True, unreal.MaterialParameterAssociation.GLOBAL_PARAMETER, True)
    if not unreal.MaterialEditingLibrary.get_material_instance_static_switch_parameter_value(prototype, SWITCH_NAME):
        raise RuntimeError("Add Puddles did not read back as enabled on the duplicate")

    for name, value in SCALAR_OVERRIDES.items():
        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(prototype, name, value)
        actual = unreal.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(prototype, name)
        unreal.log("[PuddleScalePrototype] {} requested={}, readback={}".format(name, value, actual))
        if actual is None or abs(float(actual) - value) > 0.001:
            raise RuntimeError("{} did not read back as {} (got {})".format(name, value, actual))

    if not unreal.EditorAssetLibrary.save_asset(DESTINATION):
        raise RuntimeError("Could not save the separate prototype asset")

    original_enabled = unreal.MaterialEditingLibrary.get_material_instance_static_switch_parameter_value(
        source, SWITCH_NAME)
    if original_enabled:
        raise RuntimeError("The authored landscape instance was unexpectedly modified")
    unreal.log("[PuddleScalePrototype] Saved {}; authored switch remains {}".format(
        prototype.get_path_name(), original_enabled))


try:
    main()
except Exception:
    unreal.log_error("[PuddleScalePrototype] Creation failed:\n{}".format(traceback.format_exc()))
