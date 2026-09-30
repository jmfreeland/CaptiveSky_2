"""Create a separate landscape instance with the existing puddle branch enabled."""

import traceback
import unreal


SOURCE = "/Game/Materials/MI_Island_Landscape"
DESTINATION = "/Game/Materials/MI_Island_Landscape_WetPrototype"
SWITCH_NAME = "Add Puddles"


def main():
    if unreal.EditorAssetLibrary.does_asset_exist(DESTINATION):
        raise RuntimeError("Refusing to overwrite existing prototype: " + DESTINATION)

    source = unreal.load_asset(SOURCE)
    if not source:
        raise RuntimeError("Could not load source material instance: " + SOURCE)
    switch_names = [str(name) for name in unreal.MaterialEditingLibrary.get_static_switch_parameter_names(source)]
    unreal.log("[PuddlePrototype] Source static switch parameters: {}".format(switch_names))
    if SWITCH_NAME not in switch_names:
        raise RuntimeError("Add Puddles is not exposed on the assigned material instance")
    if unreal.MaterialEditingLibrary.get_material_instance_static_switch_parameter_value(source, SWITCH_NAME):
        raise RuntimeError("Source instance already has Add Puddles enabled; expected authored source off")

    prototype = unreal.EditorAssetLibrary.duplicate_asset(SOURCE, DESTINATION)
    if not prototype:
        raise RuntimeError("Could not duplicate source instance")

    setter_result = unreal.MaterialEditingLibrary.set_material_instance_static_switch_parameter_value(
        prototype,
        SWITCH_NAME,
        True,
        unreal.MaterialParameterAssociation.GLOBAL_PARAMETER,
        True)
    unreal.MaterialEditingLibrary.update_material_instance(prototype)
    enabled = unreal.MaterialEditingLibrary.get_material_instance_static_switch_parameter_value(
        prototype, SWITCH_NAME)
    unreal.log("[PuddlePrototype] Setter returned {}; readback={}".format(setter_result, enabled))
    if not enabled:
        raise RuntimeError("Add Puddles did not read back as enabled on the duplicate instance")
    if not unreal.EditorAssetLibrary.save_asset(DESTINATION):
        raise RuntimeError("Could not save the separate prototype asset")

    parent = prototype.get_editor_property("parent")
    original_enabled = unreal.MaterialEditingLibrary.get_material_instance_static_switch_parameter_value(
        source, SWITCH_NAME)
    if not enabled or original_enabled:
        raise RuntimeError("Post-save verification failed: prototype={}, original={}".format(
            enabled, original_enabled))

    unreal.log("[PuddlePrototype] Saved {}".format(prototype.get_path_name()))
    unreal.log("[PuddlePrototype] Parent remains {}; Add Puddles prototype={}, authored={}".format(
        parent.get_path_name() if parent else "<none>", enabled, original_enabled))


try:
    main()
except Exception:
    unreal.log_error("[PuddlePrototype] Creation failed:\n{}".format(traceback.format_exc()))
