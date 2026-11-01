"""Stage a collision-preserving RatHollow V9 review in a disposable Island copy.

Run from Unreal's Python console with:
  CAPTIVESKY_RATHOLLOW_REVIEW_CHECKOUT_ROOT=<this source checkout>
  CAPTIVESKY_RATHOLLOW_REVIEW_STAGE=import|duplicate|place

Import and duplicate never load or save the source Island. The place stage
requires an audited candidate-site record, and will only save the already-open
review copy. No canonical wildlife-home or resident data is written.
"""
import json
import math
import os
import re
import runpy
import traceback
import time

import unreal


PREFIX = "[RatHollowV9Review] "
SOURCE_MAP = "/Game/Maps/Island"
DEST_FOLDER = "/Game/Generated/ComfyBlender/RatHollowHiddenV9Review/Maps"
DEST_MAP = DEST_FOLDER + "/Island_RatHollowV9Review"
DESTINATION_NAME = "RatHollowHiddenV9Review"
LIBRARY_NAME = "RatHollowV9ReviewLibrary"
ASSET_NAME = "RatHollow_HiddenV9"
FBX_NAME = "SM_RatHollow_HiddenV9_01.fbx"
MESH_NAME = "SM_RatHollow_HiddenV9_01"
MESH_PATH = ("/Game/Generated/ComfyBlender/" + DESTINATION_NAME + "/" +
             MESH_NAME + "." + MESH_NAME)
ACTOR_LABEL = "Review_RatHollow_HiddenV9"
REVIEW_DATA_RELATIVE = os.path.join("Saved", "Playtests", "Codex_RatHollowV9Review")
EXPECTED_COLLISION_COUNT = 5


def log(message):
    unreal.log(PREFIX + str(message))


def require(value, message):
    if not value:
        raise RuntimeError(message)
    return value


def checkout_root():
    root = os.path.realpath(os.environ.get("CAPTIVESKY_RATHOLLOW_REVIEW_CHECKOUT_ROOT", ""))
    require(root and os.path.isdir(root),
            "Set CAPTIVESKY_RATHOLLOW_REVIEW_CHECKOUT_ROOT to the source checkout")
    return root


def project_root():
    root = os.path.realpath(unreal.Paths.project_dir())
    require(os.path.isdir(root), "The active Unreal project directory is unavailable")
    return root


def asset_directory(root):
    return os.path.join(root, "Saved", "CaptiveSky", "ComfyBlender", ASSET_NAME)


def load_source_manifest(root):
    directory = asset_directory(root)
    fbx = os.path.join(directory, FBX_NAME)
    manifest_path = os.path.join(directory, "manifest.json")
    require(os.path.isfile(fbx), "Verified RatHollow V9 FBX is missing: " + fbx)
    require(os.path.isfile(manifest_path), "RatHollow V9 manifest is missing: " + manifest_path)
    with open(manifest_path, "r", encoding="utf-8") as handle:
        manifest = json.load(handle)
    require(manifest.get("asset") == MESH_NAME,
            "Unexpected RatHollow V9 manifest asset: {}".format(manifest.get("asset")))
    collisions = manifest.get("collision", [])
    require(len(collisions) == EXPECTED_COLLISION_COUNT and
            all(str(item).startswith("UBX_") for item in collisions),
            "RatHollow V9 must retain its five authored box collisions")
    require(manifest.get("anchors_cm", {}).get("entrance") == [-132.0, 0.0, 66.0],
            "RatHollow V9 entrance anchor differs from the reviewed source")
    return manifest


def validate_imported_mesh(root, manifest):
    mesh = require(unreal.load_asset(MESH_PATH), "Imported RatHollow V9 mesh is missing: " + MESH_PATH)
    require(isinstance(mesh, unreal.StaticMesh), "RatHollow V9 import is not a Static Mesh")
    report_path = os.path.join(asset_directory(root), "unreal_import.json")
    require(os.path.isfile(report_path), "Manifest-aware Unreal import report is missing: " + report_path)
    with open(report_path, "r", encoding="utf-8") as handle:
        report = json.load(handle)
    require(not report.get("validation_errors"), "RatHollow V9 import report has validation errors")
    require(report.get("static_mesh") == MESH_PATH,
            "Import report refers to a different mesh destination")
    require(report.get("destination", "").endswith("/" + DESTINATION_NAME),
            "Import report does not refer to the isolated RatHollow V9 destination")
    collisions = report.get("collision_shapes", [])
    require(len(collisions) == len(manifest["collision"]),
            "Imported mesh does not preserve all five authored collision boxes")
    log("VALIDATED mesh={} size_cm={} collision_boxes={}".format(
        MESH_PATH, report.get("size_cm_xyz"), len(collisions)))
    return mesh


def import_candidate():
    root = checkout_root()
    manifest = load_source_manifest(root)
    if unreal.EditorAssetLibrary.does_asset_exist(MESH_PATH):
        validate_imported_mesh(root, manifest)
        return

    previous = {
        key: os.environ.get(key)
        for key in ("COMFYBLENDER_ASSET", "COMFYBLENDER_DESTINATION",
                    "COMFYBLENDER_LIBRARY_DESTINATION", "COMFYBLENDER_SOURCE_ROOT")
    }
    try:
        os.environ["COMFYBLENDER_SOURCE_ROOT"] = os.path.join(
            root, "Saved", "CaptiveSky", "ComfyBlender")
        os.environ["COMFYBLENDER_ASSET"] = ASSET_NAME
        os.environ["COMFYBLENDER_DESTINATION"] = DESTINATION_NAME
        os.environ["COMFYBLENDER_LIBRARY_DESTINATION"] = LIBRARY_NAME
        importer = os.path.join(root, "Scripts", "Import-ComfyBlenderAsset.py")
        require(os.path.isfile(importer), "Manifest-aware asset importer is missing: " + importer)
        runpy.run_path(importer)
    finally:
        for key, value in previous.items():
            if value is None:
                os.environ.pop(key, None)
            else:
                os.environ[key] = value

    validate_imported_mesh(root, manifest)
    log("COMPLETE stage=import destination={} shared_library={}".format(
        DESTINATION_NAME, LIBRARY_NAME))


def duplicate_island():
    assets = unreal.EditorAssetLibrary
    require(assets.does_asset_exist(SOURCE_MAP), "Source Island map is missing")
    require(not assets.does_asset_exist(DEST_MAP),
            "Refusing to overwrite existing review map: " + DEST_MAP)
    assets.make_directory(DEST_FOLDER)
    require(assets.does_directory_exist(DEST_FOLDER), "Could not create RatHollow review-map folder")
    duplicate = require(assets.duplicate_asset(SOURCE_MAP, DEST_MAP),
                        "Could not create isolated RatHollow review-map copy")
    require(assets.save_loaded_asset(duplicate), "Could not save isolated RatHollow review-map copy")
    log("COMPLETE stage=duplicate source={} review={} source_saved=False".format(SOURCE_MAP, DEST_MAP))


def parse_audited_site(audit_text, site_record):
    candidate_index = site_record.get("candidate_index")
    require(isinstance(candidate_index, int) and candidate_index > 0,
            "Site record candidate_index must be a positive integer")
    pattern = re.compile(
        r"RatHollowSiteAudit candidate\[{}\] pos=(.*?) slope=".format(candidate_index))
    match = pattern.search(audit_text)
    require(match, "Selected site is not a qualified candidate in the supplied fresh audit log")
    audited_values = [float(value) for value in re.findall(r"[-+]?(?:\d+\.?\d*|\.\d+)", match.group(1))]
    location = site_record.get("location_cm")
    require(len(audited_values) == 3 and isinstance(location, list) and len(location) == 3,
            "The audit candidate and site record must each contain XYZ coordinates")
    require(all(isinstance(value, (int, float)) and math.isfinite(value) for value in location),
            "Site XYZ coordinates must be finite numbers")
    require(all(abs(float(actual) - expected) <= 2.0
                for actual, expected in zip(audited_values, location)),
            "Site coordinates do not match the selected RatHollowSiteAudit candidate")
    rotation = site_record.get("rotation_degrees", [0.0, 0.0, 0.0])
    require(isinstance(rotation, list) and len(rotation) == 3 and
            all(isinstance(value, (int, float)) and math.isfinite(value) for value in rotation),
            "Site rotation_degrees must contain three finite numbers")
    return [float(value) for value in location], [float(value) for value in rotation]


def read_review_site():
    data_root = os.path.join(project_root(), REVIEW_DATA_RELATIVE)
    site_path = os.path.join(data_root, "site.json")
    require(os.path.isfile(site_path),
            "No RatHollow review site selected; create the documented site.json after a fresh audit")
    with open(site_path, "r", encoding="utf-8") as handle:
        site_record = json.load(handle)
    require(isinstance(site_record.get("site_id"), str) and site_record["site_id"].strip(),
            "Site record needs a non-empty site_id")
    audit_relative = site_record.get("audit_log")
    require(isinstance(audit_relative, str) and audit_relative,
            "Site record needs a fresh audit_log path relative to the active project")
    active_project = project_root()
    audit_path = os.path.realpath(os.path.join(active_project, audit_relative))
    saved_prefix = os.path.realpath(os.path.join(active_project, "Saved")) + os.sep
    require(audit_path.startswith(saved_prefix) and os.path.isfile(audit_path),
            "RatHollow site audit must be a log file inside the active project's Saved tree")
    audit_age = time.time() - os.path.getmtime(audit_path)
    require(0.0 <= audit_age <= 24.0 * 60.0 * 60.0,
            "RatHollow site audit must be no more than 24 hours old; run a fresh site scan")
    with open(audit_path, "rb") as handle:
        handle.seek(0, os.SEEK_END)
        size = handle.tell()
        handle.seek(max(0, size - 4 * 1024 * 1024), os.SEEK_SET)
        audit_text = handle.read().decode("utf-8", errors="replace")
    require("RatHollowSiteAudit scan center=" in audit_text,
            "Audit log does not contain an Island.RatHollowSiteAudit scan")
    location, rotation = parse_audited_site(audit_text, site_record)
    return site_record, audit_relative, location, rotation


def place_candidate():
    root = checkout_root()
    manifest = load_source_manifest(root)
    mesh = validate_imported_mesh(root, manifest)
    world = require(unreal.EditorLevelLibrary.get_editor_world(), "No editor world is active")
    package_path = world.get_outermost().get_name()
    require(package_path == DEST_MAP,
            "Open only the isolated RatHollow review map before placement; current map is " + package_path)

    site_record, audit_relative, location, rotation = read_review_site()
    output_path = os.path.join(project_root(), REVIEW_DATA_RELATIVE, "review_report.json")
    require(not os.path.exists(output_path),
            "Refusing to overwrite existing RatHollow review report: " + output_path)
    actor_tools = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    require(actor_tools, "Editor Actor Subsystem is unavailable")
    actors = actor_tools.get_all_level_actors()
    require(not any(actor.get_actor_label() == ACTOR_LABEL for actor in actors),
            "Refusing to create a duplicate RatHollow review actor")

    actor = require(actor_tools.spawn_actor_from_object(
        mesh, unreal.Vector(*location), unreal.Rotator(*rotation), False),
        "Could not spawn RatHollow V9 review candidate")
    actor.set_actor_label(ACTOR_LABEL)
    component = require(actor.get_component_by_class(unreal.StaticMeshComponent),
                        "Review actor has no Static Mesh component")
    require(component.get_static_mesh() == mesh, "Review actor mesh assignment changed unexpectedly")
    # Preserve the FBX-authored UBX collision. The copied-map review must expose
    # capsule and navigation problems rather than hide them with NoCollision.
    require(unreal.EditorLevelLibrary.save_current_level(),
            "Could not save the isolated RatHollow review map")

    report = {
        "source_map": SOURCE_MAP,
        "review_map": DEST_MAP,
        "main_island_saved": False,
        "canonical_home_written": False,
        "resident_memory_written": False,
        "actor_label": ACTOR_LABEL,
        "mesh": MESH_PATH,
        "site_id": site_record["site_id"],
        "site_candidate_index": site_record["candidate_index"],
        "site_location_cm": location,
        "site_rotation_degrees": rotation,
        "site_audit_log": audit_relative,
        "preserved_collision_boxes": EXPECTED_COLLISION_COUNT,
    }
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, "w", encoding="utf-8") as handle:
        json.dump(report, handle, indent=2)
    log("COMPLETE stage=place " + json.dumps(report, sort_keys=True))


def main():
    stage = os.environ.get("CAPTIVESKY_RATHOLLOW_REVIEW_STAGE", "").strip().lower()
    if stage == "import":
        import_candidate()
    elif stage == "duplicate":
        duplicate_island()
    elif stage == "place":
        place_candidate()
    else:
        log("No stage selected; no assets or maps were changed")


if os.environ.get("CAPTIVESKY_RATHOLLOW_REVIEW_STAGE", "").strip():
    try:
        main()
    except Exception:
        unreal.log_error(PREFIX + "FAILED " + traceback.format_exc())
        raise
