"""Duplicate the generated wet landscape graph with broader, sparser puddle masks."""

import traceback
import unreal


SOURCE_PATH = "/Game/Materials/M_Island_Textured_Wet"
PROTOTYPES = (
    ("/Game/Materials/M_Island_Textured_Wet_BroadPools", {
        "PuddleTileCm": 1800.0,
        "PuddleCoverage": 0.28,
        "PuddleSharpness": 5.0,
    }),
    ("/Game/Materials/M_Island_Textured_Wet_BalancedPools", {
        "PuddleTileCm": 1300.0,
        "PuddleCoverage": 0.34,
        "PuddleSharpness": 6.0,
    }),
    ("/Game/Materials/M_Island_Textured_Wet_GentlePools", {
        "PuddleTileCm": 1200.0,
        "PuddleCoverage": 0.36,
        "PuddleSharpness": 6.5,
    }),
)


def create_prototype(destination_path, scalar_overrides):
    if unreal.EditorAssetLibrary.does_asset_exist(destination_path):
        unreal.log("[WetMaterialPrototype] Keeping existing capture-only asset {}".format(destination_path))
        return
    source = unreal.load_asset(SOURCE_PATH)
    if not source or source.get_class().get_name() != "Material":
        raise RuntimeError("Could not load generated wet landscape material: " + SOURCE_PATH)

    prototype = unreal.EditorAssetLibrary.duplicate_asset(SOURCE_PATH, destination_path)
    if not prototype:
        raise RuntimeError("Could not duplicate " + SOURCE_PATH)

    matches = {name: [] for name in scalar_overrides}
    for expression in unreal.MaterialEditingLibrary.get_material_expressions(prototype):
        if expression.get_class().get_name() != "MaterialExpressionScalarParameter":
            continue
        name = str(expression.get_editor_property("parameter_name"))
        if name in scalar_overrides:
            matches[name].append(expression)

    for name, value in scalar_overrides.items():
        if len(matches[name]) != 1:
            raise RuntimeError("Expected one {} parameter; found {}".format(name, len(matches[name])))
        matches[name][0].set_editor_property("default_value", value)

    errors = unreal.MaterialEditingLibrary.recompile_material(prototype)
    if errors:
        raise RuntimeError("Material compile errors: " + " | ".join(str(error) for error in errors))
    if not unreal.EditorAssetLibrary.save_asset(destination_path):
        raise RuntimeError("Could not save capture-only prototype: " + destination_path)

    for name, requested in scalar_overrides.items():
        actual = matches[name][0].get_editor_property("default_value")
        if abs(float(actual) - requested) > 0.001:
            raise RuntimeError("{} read back as {}, expected {}".format(name, actual, requested))
        unreal.log("[WetMaterialPrototype] {} requested={}, readback={}".format(name, requested, actual))
    unreal.log("[WetMaterialPrototype] Saved separate capture-only asset {}; authored graph and map assignment unchanged".format(
        destination_path))


def main():
    for destination_path, scalar_overrides in PROTOTYPES:
        create_prototype(destination_path, scalar_overrides)


try:
    main()
except Exception:
    unreal.log_error("[WetMaterialPrototype] Creation failed:\n{}".format(traceback.format_exc()))
