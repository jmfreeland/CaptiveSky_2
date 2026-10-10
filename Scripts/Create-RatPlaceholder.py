"""Create /Game/Agents/BP_Rat_Placeholder: a resident body for the rat, cloned from BP_Agent_Placeholder.

Swaps the mesh for SK_Rat_01 (Fox skeleton), scales it to a rat (~0.28), shrinks the capsule and slows the walk,
(plays the Fox breathing idle in a loop; locomotion animation still needs an AnimBP or controller). Nothing spawns it; the rat's resident folder (docs/residents/Agent_Rat_01) is still a
draft. Label its actor "Rat", never "Fenrus" (docs/residents/README.md).
  UnrealEditor-Cmd <uproject> -ExecutePythonScript=Scripts/Create-RatPlaceholder.py -RenderOffscreen -NoZen
"""
import traceback

import unreal

SRC = "/Game/Agents/BP_Agent_Placeholder"
DST = "/Game/Agents/BP_Rat_Placeholder"
MESH = "/Game/Characters/Creatures/Rat/SK_Rat_01"
IDLE = "/Game/AnimalVarietyPack/Fox/Animations/ANIM_Fox_IdleBreathe"
SCALE = 0.28


def log(m):
    unreal.log("RatBP: " + str(m))


try:
    ea = unreal.EditorAssetLibrary
    bp = unreal.load_asset(DST) if ea.does_asset_exist(DST) else ea.duplicate_asset(SRC, DST)   # idempotent: re-running re-applies the settings
    if not bp:
        raise RuntimeError("could not create " + DST)
    cdo = unreal.get_default_object(bp.generated_class())
    mesh = unreal.load_asset(MESH)
    b = mesh.get_bounds()
    cap = cdo.get_editor_property("capsule_component")
    half_h = max(12.0, b.box_extent.z * SCALE)
    cap.set_editor_property("capsule_half_height", half_h)
    cap.set_editor_property("capsule_radius", max(8.0, b.box_extent.y * SCALE * 0.35))
    comp = cdo.get_editor_property("mesh")
    comp.set_editor_property("skeletal_mesh_asset", mesh)
    comp.set_editor_property("relative_scale3d", unreal.Vector(SCALE, SCALE, SCALE))
    # mesh bounds-based placement: lowest point of the (scaled) mesh sits on the capsule bottom; fox forward is -Y in
    # mesh space so yaw it +90 to face the character's +X.
    bottom = (b.origin.z - b.box_extent.z) * SCALE            # lowest point of the scaled mesh, in the component's frame
    comp.set_editor_property("relative_location", unreal.Vector(0, 0, -half_h - bottom))
    comp.set_editor_property("relative_rotation", unreal.Rotator(roll=0, pitch=0, yaw=90))
    comp.set_editor_property("animation_mode", unreal.AnimationMode.ANIMATION_SINGLE_NODE)
    data = comp.get_editor_property("animation_data")
    data.set_editor_property("anim_to_play", unreal.load_asset(IDLE))
    data.set_editor_property("saved_looping", True)
    data.set_editor_property("saved_playing", True)
    comp.set_editor_property("animation_data", data)
    mv = cdo.get_editor_property("character_movement")
    mv.set_editor_property("max_walk_speed", 140.0)
    ea.save_asset(DST)
    log("created {} mesh={} capsule_half_height={:.1f}".format(DST, MESH, half_h))
except Exception:
    unreal.log_error("RatBP FAILED\n" + traceback.format_exc())
