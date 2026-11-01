"""Import woodland understory candidates into a disposable Island review map.

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


PREFIX = "[WoodlandUnderstoryReview] "
SOURCE_MAP = "/Game/Maps/Island"
DEST_FOLDER = "/Game/Generated/ComfyBlender/WoodlandUnderstoryReview_20261101/Maps"
DEST_MAP = DEST_FOLDER + "/Island_WoodlandUnderstory_20261101"
OLD_CLUSTER_LABEL = "WoodlandFungusCluster_Review_01"
CANDIDATES = (
    {
        "name": "WoodlandFlyAgaricCluster",
        "filename": "SM_WoodlandFlyAgaricCluster_01.fbx",
        "mesh_path": "/Game/Generated/ComfyBlender/WoodlandFlyAgaricCluster/SM_WoodlandFlyAgaricCluster_01.SM_WoodlandFlyAgaricCluster_01",
        "actor_label": "Preview_WoodlandFlyAgaricCluster",
        "offset_x_cm": 160.0,
    },
    {
        "name": "WoodlandFernCluster",
        "filename": "SM_WoodlandFernCluster_01.fbx",
        "mesh_path": "/Game/Generated/ComfyBlender/WoodlandFernCluster/SM_WoodlandFernCluster_01.SM_WoodlandFernCluster_01",
        "actor_label": "Preview_WoodlandFernCluster",
        "offset_x_cm": -190.0,
    },
    {
        "name": "WoodlandDogViolet",
        "filename": "SM_WoodlandDogViolet_01.fbx",
        "mesh_path": "/Game/Generated/ComfyBlender/WoodlandDogViolet/SM_WoodlandDogViolet_01.SM_WoodlandDogViolet_01",
        "actor_label": "Preview_WoodlandDogViolet",
        "offset_x_cm": 0.0,
        "offset_y_cm": 145.0,
    },
)
REVIEW_REPORT = "WoodlandUnderstoryReview_20261101/unreal_review.json"


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


def validate_imported_candidate(root, candidate):
    mesh = require(unreal.load_asset(candidate["mesh_path"]),
                   "Imported candidate Static Mesh is missing: " + candidate["mesh_path"])
    require(isinstance(mesh, unreal.StaticMesh),
            "Imported candidate is not a Static Mesh: " + candidate["mesh_path"])
    asset_dir = os.path.join(root, "Saved", "CaptiveSky", "ComfyBlender", candidate["name"])
    report_path = os.path.join(asset_dir, "unreal_import.json")
    require(os.path.isfile(report_path), "Unreal import report is missing: " + report_path)
    with open(report_path, "r", encoding="utf-8") as handle:
        report = json.load(handle)
    require(not report.get("validation_errors"),
            "Imported mesh failed manifest validation: " + candidate["name"])
    log("COMPLETE stage=import name={} mesh={} size_cm={} collision_shapes={}".format(
        candidate["name"], mesh.get_path_name(), report.get("size_cm_xyz"),
        len(report.get("collision_shapes", []))))


def import_candidates():
    root = project_root()
    previous = {
        key: os.environ.get(key)
        for key in ("COMFYBLENDER_ASSET", "COMFYBLENDER_DESTINATION", "COMFYBLENDER_SOURCE_ROOT")
    }
    os.environ["COMFYBLENDER_SOURCE_ROOT"] = os.path.join(root, "Saved", "CaptiveSky", "ComfyBlender")
    try:
        for candidate in CANDIDATES:
            asset_dir = os.path.join(root, "Saved", "CaptiveSky", "ComfyBlender", candidate["name"])
            fbx_path = os.path.join(asset_dir, candidate["filename"])
            require(os.path.isfile(fbx_path), "Verified candidate FBX is missing: " + fbx_path)

        for candidate in CANDIDATES:
            if unreal.EditorAssetLibrary.does_asset_exist(candidate["mesh_path"]):
                # A previous run may have imported the first candidate before
                # another import failed. Reuse only a fully validated asset;
                # never overwrite or silently trust a partial import.
                validate_imported_candidate(root, candidate)
                continue
            asset_dir = os.path.join(root, "Saved", "CaptiveSky", "ComfyBlender", candidate["name"])
            # Reuse the manifest-aware importer so units, collision, materials,
            # and the report follow the established asset pipeline.
            os.environ["COMFYBLENDER_ASSET"] = candidate["name"]
            os.environ["COMFYBLENDER_DESTINATION"] = candidate["name"]
            runpy.run_path(os.path.join(root, "Scripts", "Import-ComfyBlenderAsset.py"))
            validate_imported_candidate(root, candidate)
    finally:
        for key, value in previous.items():
            if value is None:
                os.environ.pop(key, None)
            else:
                os.environ[key] = value

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


def place_candidates():
    assets = unreal.EditorAssetLibrary
    require(assets.does_asset_exist(DEST_MAP), "Run the duplicate stage first")
    for candidate in CANDIDATES:
        require(assets.does_asset_exist(candidate["mesh_path"]),
                "Run the import stage first; missing " + candidate["mesh_path"])
    require(unreal.EditorLoadingAndSavingUtils.load_map(DEST_MAP),
            "Could not load isolated review map")
    actor_tools = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    require(actor_tools, "Editor Actor Subsystem is unavailable")
    actors = actor_tools.get_all_level_actors()
    old_matches = [actor for actor in actors if actor.get_actor_label() == OLD_CLUSTER_LABEL]
    require(len(old_matches) == 1,
            "Expected exactly one existing fungus cluster; found {}".format(len(old_matches)))
    existing_labels = {actor.get_actor_label() for actor in actors}
    require(not any(candidate["actor_label"] in existing_labels for candidate in CANDIDATES),
            "Refusing to add duplicate comparison actors")
    old_actor = old_matches[0]
    old_component = old_actor.get_component_by_class(unreal.StaticMeshComponent)
    require(old_component and old_component.get_static_mesh(),
            "Existing fungus review actor has no Static Mesh")
    old_location = old_actor.get_actor_location()
    placed_candidates = []
    for candidate in CANDIDATES:
        new_mesh = require(unreal.load_asset(candidate["mesh_path"]),
                           "Imported candidate mesh is missing: " + candidate["name"])
        new_location = unreal.Vector(old_location.x + candidate["offset_x_cm"],
                                     old_location.y + candidate.get("offset_y_cm", 0.0),
                                     old_location.z)
        new_actor = require(actor_tools.spawn_actor_from_object(
            new_mesh, new_location, old_actor.get_actor_rotation(), False),
            "Could not spawn comparison candidate: " + candidate["name"])
        new_actor.set_actor_label(candidate["actor_label"])
        new_component = new_actor.get_component_by_class(unreal.StaticMeshComponent)
        require(new_component, "Comparison actor has no Static Mesh component: " + candidate["name"])
        new_component.set_static_mesh(new_mesh)
        new_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        new_actor.set_actor_hidden_in_game(False)
        placed_candidates.append({
            "candidate_actor": candidate["actor_label"],
            "candidate_mesh": new_mesh.get_path_name(),
            "offset_x_cm": candidate["offset_x_cm"],
            "offset_y_cm": candidate.get("offset_y_cm", 0.0),
            "candidate_collision": "NoCollision",
        })
    require(unreal.EditorLevelLibrary.save_current_level(),
            "Could not save the isolated understory review map")

    report = {
        "source_map": SOURCE_MAP,
        "review_map": DEST_MAP,
        "existing_actor": OLD_CLUSTER_LABEL,
        "existing_mesh": old_component.get_static_mesh().get_path_name(),
        "candidates": placed_candidates,
        "main_island_saved": False,
    }
    report_path = os.path.join(project_root(), "Saved", "CaptiveSky", "ComfyBlender", REVIEW_REPORT)
    os.makedirs(os.path.dirname(report_path), exist_ok=True)
    with open(report_path, "w", encoding="utf-8") as handle:
        json.dump(report, handle, indent=2)
    log("COMPLETE stage=place " + json.dumps(report, sort_keys=True))


def main():
    stage = os.environ.get("CAPTIVESKY_FUNGUS_REVIEW_STAGE", "").strip().lower()
    if stage == "import":
        import_candidates()
    elif stage == "duplicate":
        duplicate_island()
    elif stage == "place":
        place_candidates()
    else:
        log("No stage selected; no assets or maps were changed")


if os.environ.get("CAPTIVESKY_FUNGUS_REVIEW_STAGE", "").strip():
    try:
        main()
    except Exception:
        unreal.log_error(PREFIX + "FAILED " + traceback.format_exc())
        raise
