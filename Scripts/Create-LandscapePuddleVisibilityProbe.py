"""Create a deliberately exaggerated, isolated puddle-mask visibility test material."""

import traceback
import unreal


SOURCE = "/Game/Materials/MI_Island_Landscape"
DESTINATION = "/Game/Materials/MI_Island_Landscape_PuddleVisibilityProbe"
SWITCH_NAME = "Add Puddles"
SCALAR_OVERRIDES = {
    "Puddle Size": 25.0,
    "Puddle Depth": 20.0,
    "Puddle Clarity": 0.1,
    "Puddle Constrain": 1.0,
}


def main():
    if unreal.EditorAssetLibrary.does_asset_exist(DESTINATION):
        raise RuntimeError("Refusing to overwrite existing probe: " + DESTINATION)

    source = unreal.load_asset(SOURCE)
    if not source:
        raise RuntimeError("Could not load source material instance: " + SOURCE)
    if unreal.MaterialEditingLibrary.get_material_instance_static_switch_parameter_value(source, SWITCH_NAME):
        raise RuntimeError("Expected the authored Add Puddles switch to remain disabled")

    probe = unreal.EditorAssetLibrary.duplicate_asset(SOURCE, DESTINATION)
    if not probe:
        raise RuntimeError("Could not duplicate source instance")

    unreal.MaterialEditingLibrary.set_material_instance_static_switch_parameter_value(
        probe, SWITCH_NAME, True, unreal.MaterialParameterAssociation.GLOBAL_PARAMETER, True)
    if not unreal.MaterialEditingLibrary.get_material_instance_static_switch_parameter_value(probe, SWITCH_NAME):
        raise RuntimeError("Add Puddles did not read back as enabled on the duplicate")

    for name, value in SCALAR_OVERRIDES.items():
        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(probe, name, value)
        actual = unreal.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(probe, name)
        unreal.log("[PuddleVisibilityProbe] {} requested={}, readback={}".format(name, value, actual))
        if actual is None or abs(float(actual) - value) > 0.001:
            raise RuntimeError("{} did not read back as {} (got {})".format(name, value, actual))

    unreal.MaterialEditingLibrary.update_material_instance(probe)
    if not unreal.EditorAssetLibrary.save_asset(DESTINATION):
        raise RuntimeError("Could not save the separate visibility probe")

    if unreal.MaterialEditingLibrary.get_material_instance_static_switch_parameter_value(source, SWITCH_NAME):
        raise RuntimeError("The authored landscape instance was unexpectedly modified")
    unreal.log("[PuddleVisibilityProbe] Saved {}; authored switch remains disabled".format(
        probe.get_path_name()))


try:
    main()
except Exception:
    unreal.log_error("[PuddleVisibilityProbe] Creation failed:\n{}".format(traceback.format_exc()))
