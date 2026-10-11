"""Create resident bodies for the human actors from their built MetaHumans:
/Game/Agents/BP_Aster_MetaHuman and /Game/Agents/BP_Innkeeper_MetaHuman, cloned from BP_Agent_Placeholder.

The character mesh becomes the MetaHuman body skeletal mesh; a Face skeletal mesh component is attached to it
and follows it (leader pose). Run Create-MetaHumans.py and then build both characters in the MetaHuman Character
editor first (docs/findings/2026-10-10-creature-and-metahuman-bodies.md); the build lands in /Game/Characters/MetaHumans/Built.
  UnrealEditor-Cmd <uproject> -ExecutePythonScript=Scripts/Create-MetaHumanBodies.py -RenderOffscreen -NoZen
"""
import traceback

import unreal

SRC = "/Game/Agents/BP_Agent_Placeholder"
RESIDENTS = [("Aster", "BP_Aster_MetaHuman"), ("Innkeeper", "BP_Innkeeper_MetaHuman")]


def log(m):
    unreal.log("MetaHumanBody: " + str(m))


def make(name, who):
    ea = unreal.EditorAssetLibrary
    base = "/Game/Characters/MetaHumans/Built/{0}/MHC_{0}".format(who)
    body = unreal.load_asset("{}/Body/SKM_MHC_{}_BodyMesh".format(base, who))
    face = unreal.load_asset("{}/Face/SKM_MHC_{}_FaceMesh".format(base, who))
    if not body or not face:
        raise RuntimeError("built MetaHuman meshes for {} are missing under {}".format(who, base))
    dst = "/Game/Agents/" + name
    if ea.does_asset_exist(dst):
        ea.delete_asset(dst)
    bp = ea.duplicate_asset(SRC, dst)
    if not bp:
        raise RuntimeError("could not create " + dst)
    cdo = unreal.get_default_object(bp.generated_class())
    cdo.get_editor_property("mesh").set_editor_property("skeletal_mesh_asset", body)
    cdo.get_editor_property("mesh").set_editor_property("animation_mode", unreal.AnimationMode.ANIMATION_SINGLE_NODE)

    sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles = sub.k2_gather_subobject_data_for_blueprint(bp)
    root = handles[0]
    lib = unreal.SubobjectDataBlueprintFunctionLibrary

    def obj(handle):
        return lib.get_object_for_blueprint(lib.get_data(handle) if hasattr(lib, "get_data") else sub.k2_find_subobject_data_from_handle(handle), bp)
    mesh_handle = next(h for h in handles if obj(h) is not None and obj(h).get_name() == "CharacterMesh0")
    params = unreal.AddNewSubobjectParams(parent_handle=mesh_handle, new_class=unreal.SkeletalMeshComponent, blueprint_context=bp)
    new_handle, fail = sub.add_new_subobject(params)
    sub.rename_subobject(new_handle, "Face")
    comp = obj(new_handle)
    comp.set_editor_property("skeletal_mesh_asset", face)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    ea.save_asset(dst)
    log("created " + dst)


try:
    for who, name in RESIDENTS:
        make(name, who)
except Exception:
    unreal.log_error("MetaHumanBody FAILED\n" + traceback.format_exc())
