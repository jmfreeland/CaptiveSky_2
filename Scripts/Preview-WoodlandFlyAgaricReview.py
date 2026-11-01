"""Import and place a fly-agaric candidate in a disposable Island review map.

Run from Unreal's Python console with CAPTIVESKY_FUNGUS_REVIEW_ROOT set to this
checkout and CAPTIVESKY_FUNGUS_REVIEW_STAGE set to ``import``, ``duplicate``,
or ``place``. The main Island map is never saved or modified. Each map stage
refuses to overwrite an existing review asset.
"""
import json
import os
import runpy
import traceback

import unreal


PREFIX = "[WoodlandFlyAgaricReview] "
SOURCE_MAP = "/Game/Maps/Island"
DEST_FOLDER = "/Game/Generated/ComfyBlender/WoodlandFlyAgaricReview_20261101/Maps"
DEST_MAP = DEST_FOLDER + "/Island_WoodlandFungusAB_20261101"
OLD_CLUSTER_LABEL = "WoodlandFungusCluster_Review_01"
NEW_CLUSTER_LABEL = "Preview_WoodlandFlyAgaricCluster"
ASSET_NAME = "WoodlandFlyAgaricCluster"
ASSET_DESTINATION = "/Game/Generated/ComfyBlender/" + ASSET_NAME
MESH_PATH = ASSET_DESTINATION + "/SM_WoodlandFlyAgaricCluster_01.SM_WoodlandFlyAgaricCluster_01"
COMPARISON_OFFSET_CM = 160.0


def log(message):
    unreal.log(PREFIX + str(message))


def require(value, message):
    if not value:
        raise RuntimeError(message)
    return value


def project_root():
    root = os.path.realpath(os.environ.get("CAPTIVESKY_FUNGUS_REVIEW_ROOT", ""))
    require(root and os.path.isdir(root),
            "Set CAPTIVESKY_FUNGUS_REVIEW_ROOT to the CaptiveSky_2 checkout")
    return root


def import_candidate():
    root = project_root()
    asset_dir = os.path.join(root, "Saved", "CaptiveSky", "ComfyBlender", ASSET_NAME)
    fbx_path = os.path.join(asset_dir, "SM_WoodlandFlyAgaricCluster_01.fbx")
    require(os.path.isfile(fbx_path), "Verified fly-agaric FBX is missing: " + fbx_path)
    require(not unreal.EditorAssetLibrary.does_asset_exist(MESH_PATH),
            "Refusing to reimport an existing mesh; choose a new destination for a revision")

    # Reuse the manifest-aware importer so material slots, collision, units and
    # the report follow the project's established asset pipeline.
    previous = {
        key: os.environ.get(key)
        for key in ("COMFYBLENDER_ASSET", "COMFYBLENDER_DESTINATION", "COMFYBLENDER_SOURCE_ROOT")
    }
    os.environ["COMFYBLENDER_ASSET"] = ASSET_NAME
    os.environ["COMFYBLENDER_DESTINATION"] = ASSET_NAME
    os.environ["COMFYBLENDER_SOURCE_ROOT"] = os.path.join(root, "Saved", "CaptiveSky", "ComfyBlender")
    try:
        runpy.run_path(os.path.join(root, "Scripts", "Import-ComfyBlenderAsset.py"))
    finally:
        for key, value in previous.items():
            if value is None:
                os.environ.pop(key, None)
            else:
                os.environ[key] = value

    mesh = unreal.load_asset(MESH_PATH)
    require(isinstance(mesh, unreal.StaticMesh), "Imported fly-agaric Static Mesh is missing")
    report_path = os.path.join(asset_dir, "unreal_import.json")
    with open(report_path, "r", encoding="utf-8") as handle:
        report = json.load(handle)
    require(not report.get("validation_errors"), "Imported mesh failed manifest validation")
    log("COMPLETE stage=import mesh={} size_cm={} collision_shapes={}".format(
        mesh.get_path_name(), report.get("size_cm_xyz"), len(report.get("collision_shapes", []))))


def duplicate_island():
    assets = unreal.EditorAssetLibrary
    require(assets.does_asset_exist(SOURCE_MAP), "Source Island map is missing")
    require(not assets.does_asset_exist(DEST_MAP),
            "Refusing to overwrite existing review map: " + DEST_MAP)
    assets.make_directory(DEST_FOLDER)
    require(assets.does_directory_exist(DEST_FOLDER), "Could not create review-map folder")
    duplicate = require(assets.duplicate_asset(SOURCE_MAP, DEST_MAP),
                        "Could not create isolated Island review map")
    require(assets.save_loaded_asset(duplicate), "Could not save isolated review-map copy")
    log("COMPLETE stage=duplicate source={} review={}".format(SOURCE_MAP, DEST_MAP))


def place_candidate():
    assets = unreal.EditorAssetLibrary
    require(assets.does_asset_exist(DEST_MAP), "Run the duplicate stage first")
    require(assets.does_asset_exist(MESH_PATH), "Run the import stage first")
    require(unreal.EditorLoadingAndSavingUtils.load_map(DEST_MAP),
            "Could not load isolated review map")
    actor_tools = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    require(actor_tools, "Editor Actor Subsystem is unavailable")
    actors = actor_tools.get_all_level_actors()
    old_matches = [actor for actor in actors if actor.get_actor_label() == OLD_CLUSTER_LABEL]
    require(len(old_matches) == 1,
            "Expected exactly one existing fungus cluster; found {}".format(len(old_matches)))
    require(not any(actor.get_actor_label() == NEW_CLUSTER_LABEL for actor in actors),
            "Refusing to add a duplicate comparison actor")
    old_actor = old_matches[0]
    old_component = old_actor.get_component_by_class(unreal.StaticMeshComponent)
    require(old_component and old_component.get_static_mesh(),
            "Existing fungus review actor has no Static Mesh")
    new_mesh = require(unreal.load_asset(MESH_PATH), "Imported candidate mesh is missing")

    old_location = old_actor.get_actor_location()
    new_location = unreal.Vector(old_location.x + COMPARISON_OFFSET_CM,
                                 old_location.y, old_location.z)
    new_actor = require(actor_tools.spawn_actor_from_object(
        new_mesh, new_location, old_actor.get_actor_rotation(), False),
        "Could not spawn the comparison candidate")
    new_actor.set_actor_label(NEW_CLUSTER_LABEL)
    new_component = new_actor.get_component_by_class(unreal.StaticMeshComponent)
    require(new_component, "Comparison actor has no Static Mesh component")
    new_component.set_static_mesh(new_mesh)
    new_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    new_actor.set_actor_hidden_in_game(False)
    require(unreal.EditorLevelLibrary.save_current_level(),
            "Could not save the isolated A/B review map")

    report = {
        "source_map": SOURCE_MAP,
        "review_map": DEST_MAP,
        "existing_actor": OLD_CLUSTER_LABEL,
        "existing_mesh": old_component.get_static_mesh().get_path_name(),
        "candidate_actor": NEW_CLUSTER_LABEL,
        "candidate_mesh": new_mesh.get_path_name(),
        "comparison_offset_cm": COMPARISON_OFFSET_CM,
        "candidate_collision": "NoCollision",
        "main_island_saved": False,
    }
    report_path = os.path.join(project_root(), "Saved", "CaptiveSky", "ComfyBlender",
                               ASSET_NAME, "unreal_review.json")
    with open(report_path, "w", encoding="utf-8") as handle:
        json.dump(report, handle, indent=2)
    log("COMPLETE stage=place " + json.dumps(report, sort_keys=True))


def main():
    stage = os.environ.get("CAPTIVESKY_FUNGUS_REVIEW_STAGE", "").strip().lower()
    if stage == "import":
        import_candidate()
    elif stage == "duplicate":
        duplicate_island()
    elif stage == "place":
        place_candidate()
    else:
        log("No stage selected; no assets or maps were changed")


if os.environ.get("CAPTIVESKY_FUNGUS_REVIEW_STAGE", "").strip():
    try:
        main()
    except Exception:
        unreal.log_error(PREFIX + "FAILED " + traceback.format_exc())
        raise
