"""Create a disposable Island copy with one collisionless woodland fern.

Run in separate UnrealEditor-Cmd processes by setting
CAPTIVESKY_FERN_PREVIEW_STAGE to ``duplicate`` and then ``place``. Use the
``move`` stage only to reposition the actor inside the disposable review map.
The source Island is never saved; an existing review map is never overwritten.
"""
import json
import os
import traceback

import unreal


PREFIX = "[WoodlandFernTrial] "
SOURCE_MAP = "/Game/Maps/Island"
DEST_FOLDER = "/Game/Generated/ComfyBlender/WoodlandFern_Review_20261012/Maps"
DEST_MAP = DEST_FOLDER + "/Island_WoodlandFernTrial_20261012"
MESH_PATH = "/Game/Generated/ComfyBlender/WoodlandFern_Review_20261012/SM_WoodlandFernClump_01"
PREVIEW_LABEL = "Preview_WoodlandFernClump"
PREVIEW_LOCATION = unreal.Vector(-105600.0, 104050.0, 2335.0)


def log(message):
    unreal.log(PREFIX + str(message))


def require(value, message):
    if not value:
        raise RuntimeError(message)
    return value


def duplicate_map():
    assets = unreal.EditorAssetLibrary
    if assets.does_asset_exist(DEST_MAP):
        raise RuntimeError("Refusing to overwrite isolated review map: " + DEST_MAP)
    require(assets.does_asset_exist(SOURCE_MAP), "Source Island map is missing")
    assets.make_directory(DEST_FOLDER)
    require(assets.does_directory_exist(DEST_FOLDER), "Could not create review-map folder")
    duplicate = require(assets.duplicate_asset(SOURCE_MAP, DEST_MAP),
                        "Could not duplicate Island map")
    require(assets.save_loaded_asset(duplicate), "Could not save isolated Island copy")
    log("REPORT " + json.dumps({"stage": "duplicate", "source_map": SOURCE_MAP,
                                "trial_map": DEST_MAP}, sort_keys=True))
    log("COMPLETE stage=duplicate")


def place_candidate():
    assets = unreal.EditorAssetLibrary
    require(assets.does_asset_exist(DEST_MAP), "Run duplicate stage first; review map is missing")
    require(unreal.EditorLoadingAndSavingUtils.load_map(DEST_MAP),
            "Could not load isolated review map")
    mesh = assets.load_asset(MESH_PATH)
    require(isinstance(mesh, unreal.StaticMesh), "Woodland fern Static Mesh is missing")
    actor_tools = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    require(actor_tools, "Editor Actor Subsystem is unavailable")

    existing = [actor for actor in actor_tools.get_all_level_actors()
                if actor.get_actor_label() == PREVIEW_LABEL]
    if existing:
        raise RuntimeError("Refusing duplicate preview placement; found {} actor(s)".format(
            len(existing)))

    actor = require(actor_tools.spawn_actor_from_object(
        mesh, PREVIEW_LOCATION, unreal.Rotator(0.0, 0.0, 0.0), False),
        "Could not spawn fern into the isolated review map")
    actor.set_actor_label(PREVIEW_LABEL)
    component = actor.get_component_by_class(unreal.StaticMeshComponent)
    require(component, "Preview actor has no StaticMeshComponent")
    component.set_static_mesh(mesh)
    component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    component.set_visibility(True, True)
    actor.set_actor_hidden_in_game(False)

    require(unreal.EditorLevelLibrary.save_current_level(),
            "Could not save isolated review map")
    log("REPORT " + json.dumps({"stage": "place", "trial_map": DEST_MAP,
                                "actor": PREVIEW_LABEL, "mesh": mesh.get_path_name(),
                                "location_cm": [PREVIEW_LOCATION.x, PREVIEW_LOCATION.y,
                                                PREVIEW_LOCATION.z],
                                "collision": "NoCollision"}, sort_keys=True))
    log("COMPLETE stage=place; source Island was not saved or modified")


def move_candidate():
    assets = unreal.EditorAssetLibrary
    require(assets.does_asset_exist(DEST_MAP), "Review map is missing")
    require(unreal.EditorLoadingAndSavingUtils.load_map(DEST_MAP),
            "Could not load isolated review map")
    actor_tools = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    require(actor_tools, "Editor Actor Subsystem is unavailable")
    matches = [actor for actor in actor_tools.get_all_level_actors()
               if actor.get_actor_label() == PREVIEW_LABEL]
    require(len(matches) == 1, "Expected exactly one fern review actor; found {}".format(
        len(matches)))
    actor = matches[0]
    old_location = actor.get_actor_location()
    require(actor.set_actor_location(PREVIEW_LOCATION, False, False),
            "Could not move fern within the isolated review map")
    require(unreal.EditorLevelLibrary.save_current_level(),
            "Could not save isolated review map")
    log("REPORT " + json.dumps({"stage": "move", "trial_map": DEST_MAP,
                                "actor": PREVIEW_LABEL,
                                "old_location_cm": [old_location.x, old_location.y,
                                                    old_location.z],
                                "new_location_cm": [PREVIEW_LOCATION.x,
                                                    PREVIEW_LOCATION.y,
                                                    PREVIEW_LOCATION.z]}, sort_keys=True))
    log("COMPLETE stage=move; source Island was not saved or modified")


def disable_preview_shadow():
    assets = unreal.EditorAssetLibrary
    require(assets.does_asset_exist(DEST_MAP), "Review map is missing")
    require(unreal.EditorLoadingAndSavingUtils.load_map(DEST_MAP),
            "Could not load isolated review map")
    actor_tools = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    require(actor_tools, "Editor Actor Subsystem is unavailable")
    matches = [actor for actor in actor_tools.get_all_level_actors()
               if actor.get_actor_label() == PREVIEW_LABEL]
    require(len(matches) == 1, "Expected exactly one fern review actor; found {}".format(
        len(matches)))
    component = matches[0].get_component_by_class(unreal.StaticMeshComponent)
    require(component, "Preview actor has no StaticMeshComponent")
    old_cast_shadow = bool(component.get_editor_property("cast_shadow"))
    component.set_editor_property("cast_shadow", False)
    require(unreal.EditorLevelLibrary.save_current_level(),
            "Could not save isolated review map")
    log("REPORT " + json.dumps({"stage": "shadow_off", "trial_map": DEST_MAP,
                                "old_cast_shadow": old_cast_shadow,
                                "new_cast_shadow": False}, sort_keys=True))
    log("COMPLETE stage=shadow_off; source Island was not saved or modified")


try:
    stage = os.environ.get("CAPTIVESKY_FERN_PREVIEW_STAGE", "").strip().lower()
    if stage == "duplicate":
        duplicate_map()
    elif stage == "place":
        place_candidate()
    elif stage == "move":
        move_candidate()
    elif stage == "shadow_off":
        disable_preview_shadow()
    else:
        raise RuntimeError("Set CAPTIVESKY_FERN_PREVIEW_STAGE to duplicate, place, move, or shadow_off")
except Exception:
    unreal.log_error(PREFIX + "FAILED " + traceback.format_exc())
    raise
