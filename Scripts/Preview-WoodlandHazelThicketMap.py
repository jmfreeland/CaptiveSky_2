"""Create an isolated Island-map copy with one hazel candidate for Game review.

Run in two separate UnrealEditor-Cmd processes:
  CAPTIVESKY_HAZEL_PREVIEW_STAGE=duplicate
  CAPTIVESKY_HAZEL_PREVIEW_STAGE=place

The script never saves /Game/Maps/Island. It refuses to overwrite its trial map.
"""
import json
import os
import traceback

import unreal


PREFIX = "[HazelThicketTrial] "
SOURCE_MAP = "/Game/Maps/Island"
DEST_FOLDER = "/Game/Generated/ComfyBlender/WoodlandHazelThicket_Review_20261012/Maps"
DEST_MAP = DEST_FOLDER + "/Island_WoodlandHazelThicketTrial_20261012"
MESH_PATH = "/Game/Generated/ComfyBlender/WoodlandHazelThicket_Review_20261012/SM_WoodlandHazelThicket_01"
PREVIEW_LABEL = "Preview_WoodlandHazelThicket"
PREVIEW_LOCATION = unreal.Vector(-105200.0, 104200.0, 2335.0)


def log(message):
    unreal.log(PREFIX + str(message))


def require(value, message):
    if not value:
        raise RuntimeError(message)
    return value


def duplicate_map():
    assets = unreal.EditorAssetLibrary
    if assets.does_asset_exist(DEST_MAP):
        raise RuntimeError("Refusing to overwrite isolated trial map: " + DEST_MAP)
    require(assets.does_asset_exist(SOURCE_MAP), "Source Island map is missing")
    assets.make_directory(DEST_FOLDER)
    require(assets.does_directory_exist(DEST_FOLDER), "Could not create trial map folder")
    duplicate = require(assets.duplicate_asset(SOURCE_MAP, DEST_MAP), "Could not duplicate Island map")
    require(assets.save_loaded_asset(duplicate), "Could not save isolated Island copy")
    log("REPORT " + json.dumps({"stage": "duplicate", "source_map": SOURCE_MAP,
                                "trial_map": DEST_MAP}, sort_keys=True))
    log("COMPLETE stage=duplicate")


def place_candidate():
    assets = unreal.EditorAssetLibrary
    require(assets.does_asset_exist(DEST_MAP), "Run duplicate stage first; trial map is missing")
    require(unreal.EditorLoadingAndSavingUtils.load_map(DEST_MAP), "Could not load isolated trial map")
    mesh = assets.load_asset(MESH_PATH)
    require(isinstance(mesh, unreal.StaticMesh), "Hazel review Static Mesh is missing")
    actor_tools = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    require(actor_tools, "Editor Actor Subsystem is unavailable")

    existing = [actor for actor in actor_tools.get_all_level_actors()
                if actor.get_actor_label() == PREVIEW_LABEL]
    if existing:
        raise RuntimeError("Refusing duplicate preview placement; found {} actor(s)".format(len(existing)))

    actor = require(actor_tools.spawn_actor_from_object(
        mesh, PREVIEW_LOCATION, unreal.Rotator(0.0, 0.0, 0.0), False),
        "Could not spawn review mesh into the isolated trial map")
    actor.set_actor_label(PREVIEW_LABEL)
    component = actor.get_component_by_class(unreal.StaticMeshComponent)
    require(component, "Preview actor has no StaticMeshComponent")
    component.set_static_mesh(mesh)
    component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    component.set_visibility(True, True)
    actor.set_actor_hidden_in_game(False)

    require(unreal.EditorLevelLibrary.save_current_level(), "Could not save isolated trial map")
    log("REPORT " + json.dumps({"stage": "place", "trial_map": DEST_MAP,
                                "actor": PREVIEW_LABEL, "mesh": mesh.get_path_name(),
                                "location_cm": [PREVIEW_LOCATION.x, PREVIEW_LOCATION.y, PREVIEW_LOCATION.z],
                                "collision": "NoCollision"}, sort_keys=True))
    log("COMPLETE stage=place; source Island was not saved or modified")


try:
    stage = os.environ.get("CAPTIVESKY_HAZEL_PREVIEW_STAGE", "").strip().lower()
    if stage == "duplicate":
        duplicate_map()
    elif stage == "place":
        place_candidate()
    else:
        raise RuntimeError("Set CAPTIVESKY_HAZEL_PREVIEW_STAGE to duplicate or place")
except Exception:
    unreal.log_error(PREFIX + "FAILED " + traceback.format_exc())
    raise
