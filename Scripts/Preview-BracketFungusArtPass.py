"""Stage an isolated Bracket Fungus preview map without touching the Island.

Run twice in separate UnrealEditor-Cmd processes:
  BRACKET_FUNGUS_PREVIEW_STAGE=duplicate  (duplicates the existing preview)
  BRACKET_FUNGUS_PREVIEW_STAGE=replace    (loads the duplicate and swaps its mesh)

Separating these stages avoids keeping the duplicated UWorld referenced while
the editor changes maps, which triggered UE 5.8's world-memory-leak fatal check.
"""

import json
import os
import traceback

import unreal


SOURCE_MAP = "/Game/Generated/ComfyBlender/BracketFungusLog/Maps/BracketFungusLogPreviewV2_20261012"
DEST_FOLDER = "/Game/Generated/ComfyBlender/BracketFungusLog_ArtPass_20261012/Maps"
DEST_MAP = DEST_FOLDER + "/BracketFungusLogArtPassPreview_20261012"
TARGET_MESH = "/Game/Generated/ComfyBlender/BracketFungusLog_ArtPass_20261012/SM_BracketFungusLog_01"
EXPECTED_MESH_NAME = "SM_BracketFungusLog_01"


def duplicate_map():
    assets = unreal.EditorAssetLibrary
    if assets.does_asset_exist(DEST_MAP):
        raise RuntimeError("Refusing to overwrite existing preview map: " + DEST_MAP)
    if not assets.does_asset_exist(SOURCE_MAP):
        raise RuntimeError("Source preview map missing: " + SOURCE_MAP)
    assets.make_directory(DEST_FOLDER)
    if not assets.does_directory_exist(DEST_FOLDER):
        raise RuntimeError("Could not create isolated preview folder: " + DEST_FOLDER)
    duplicate = assets.duplicate_asset(SOURCE_MAP, DEST_MAP)
    if not duplicate:
        raise RuntimeError("Could not duplicate source preview map.")
    if not assets.save_loaded_asset(duplicate):
        raise RuntimeError("Could not save the duplicated preview map.")
    report = {"stage": "duplicate", "source_map": SOURCE_MAP, "preview_map": DEST_MAP}
    unreal.log("[BracketFungusArtPassPreview] REPORT " + json.dumps(report, sort_keys=True))
    unreal.log("[BracketFungusArtPassPreview] COMPLETE stage=duplicate")


def replace_mesh():
    assets = unreal.EditorAssetLibrary
    if not assets.does_asset_exist(DEST_MAP):
        raise RuntimeError("Run the duplicate stage first; preview map missing: " + DEST_MAP)
    if not unreal.EditorLoadingAndSavingUtils.load_map(DEST_MAP):
        raise RuntimeError("Could not load isolated preview map: " + DEST_MAP)

    replacement = assets.load_asset(TARGET_MESH)
    if not isinstance(replacement, unreal.StaticMesh):
        raise RuntimeError("Art-pass Static Mesh missing: " + TARGET_MESH)

    replaced = []
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        component = getattr(actor, "static_mesh_component", None)
        if not component:
            continue
        mesh = component.get_editor_property("static_mesh")
        if mesh and mesh.get_name() == EXPECTED_MESH_NAME:
            component.set_static_mesh(replacement)
            replaced.append({
                "actor": actor.get_actor_label(),
                "location": str(actor.get_actor_location()),
                "mesh": replacement.get_path_name(),
            })

    if len(replaced) != 1:
        raise RuntimeError("Expected exactly one preview mesh replacement; got {}".format(len(replaced)))
    if not unreal.EditorLevelLibrary.save_current_level():
        raise RuntimeError("Could not save isolated preview map.")

    report = {"stage": "replace", "preview_map": DEST_MAP, "replacement": replaced}
    unreal.log("[BracketFungusArtPassPreview] REPORT " + json.dumps(report, sort_keys=True))
    unreal.log("[BracketFungusArtPassPreview] COMPLETE stage=replace")


def assign_hazel_bark_material():
    assets = unreal.EditorAssetLibrary
    if not unreal.EditorLoadingAndSavingUtils.load_map(DEST_MAP):
        raise RuntimeError("Could not load isolated preview map: " + DEST_MAP)
    material = assets.load_asset(
        "/Game/Megaplant_Library/Tree_Common_Hazel/Materials/MI_Common_Hazel_Bark_01")
    if not material:
        raise RuntimeError("Common Hazel bark material instance could not be loaded.")

    matches = []
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        if actor.get_actor_label() != "Preview_BracketFungusLog":
            continue
        component = getattr(actor, "static_mesh_component", None)
        if component:
            component.set_material(0, material)
            matches.append({"actor": actor.get_actor_label(), "material": material.get_path_name()})
    if len(matches) != 1:
        raise RuntimeError("Expected one Bracket Fungus preview actor; got {}".format(len(matches)))
    if not unreal.EditorLevelLibrary.save_current_level():
        raise RuntimeError("Could not save the isolated material-preview map.")

    report = {"stage": "assign_hazel_bark", "preview_map": DEST_MAP, "assignment": matches}
    unreal.log("[BracketFungusArtPassPreview] REPORT " + json.dumps(report, sort_keys=True))
    unreal.log("[BracketFungusArtPassPreview] COMPLETE stage=assign_hazel_bark")


try:
    stage = os.environ.get("BRACKET_FUNGUS_PREVIEW_STAGE", "").strip().lower()
    if stage == "duplicate":
        duplicate_map()
    elif stage == "replace":
        replace_mesh()
    elif stage == "assign_hazel_bark":
        assign_hazel_bark_material()
    else:
        raise RuntimeError("Set BRACKET_FUNGUS_PREVIEW_STAGE to duplicate, replace, or assign_hazel_bark.")
except Exception:
    unreal.log_error("[BracketFungusArtPassPreview] FAILED " + traceback.format_exc())
    raise
