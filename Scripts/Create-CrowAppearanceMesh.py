"""Create a separate Crow mesh asset whose default material is the Island feather variant.

Run with UE 5.8.3:
  UnrealEditor-Cmd.exe <project> -ExecutePythonScript=<this file> -unattended -NoZen -abslog=<log>

The Fab mesh is never edited. The copied mesh lives in gitignored local Content, and the Raven uses
it only when available; projects without it retain the original optional Crow mesh fallback.
"""

import traceback
import unreal


SOURCE = "/Game/AnimalVarietyPack/Crow/Meshes/SK_Crow"
DESTINATION = "/Game/AnimalVarietyPack/Crow/Meshes/SK_Crow_CaptiveSky"
MATERIAL = "/Game/AnimalVarietyPack/Crow/Materials/M_Crow_CaptiveSky"


def main():
    assets = unreal.EditorAssetLibrary
    source = unreal.load_asset(SOURCE)
    material = unreal.load_asset(MATERIAL)
    if not source or not isinstance(source, unreal.SkeletalMesh):
        raise RuntimeError("Missing or unsupported source mesh " + SOURCE)
    if not material or not isinstance(material, unreal.MaterialInterface):
        raise RuntimeError("Missing or unsupported Island material " + MATERIAL)

    b_existing_variant = assets.does_asset_exist(DESTINATION)
    variant = unreal.load_asset(DESTINATION) if b_existing_variant else None
    if not variant:
        variant = assets.duplicate_asset(SOURCE, DESTINATION)
    if not variant or not isinstance(variant, unreal.SkeletalMesh):
        raise RuntimeError("Could not load or duplicate " + SOURCE)
    slots = list(variant.get_editor_property("materials"))
    if len(slots) != 1:
        raise RuntimeError("Expected one Crow material slot; found {}".format(len(slots)))
    slots[0].set_editor_property("material_interface", material)
    variant.set_editor_property("materials", slots)
    variant.post_edit_change()
    if not assets.save_asset(DESTINATION):
        raise RuntimeError("Could not save " + DESTINATION)

    saved = unreal.load_asset(DESTINATION)
    saved_material = saved.get_editor_property("materials")[0].get_editor_property("material_interface")
    if not saved_material or saved_material.get_path_name() != material.get_path_name():
        raise RuntimeError("Saved mesh did not retain the Island material in its default slot")
    unreal.log("[CrowAppearanceMesh] Created {} with material {}".format(
        DESTINATION, saved_material.get_path_name()))


try:
    main()
except Exception:
    unreal.log_error("[CrowAppearanceMesh] FAILED " + traceback.format_exc())
    raise
