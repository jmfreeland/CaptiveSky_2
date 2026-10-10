"""Creates (or updates) the MetaHuman Character assets for the human-bodied residents.

Run in a UE 5.8 editor with MetaHumanGenerator enabled:
    UnrealEditor-Cmd <uproject> -ExecutePythonScript=Scripts/Create-MetaHumans.py -NullRHI

Uses the engine's own MetaHumanGenerator toolset helpers (simplified body / skin / eyes API). The assets are
`MetaHumanCharacter` definitions under /Game/Characters/MetaHumans; Assemble/Build (which needs the MetaHuman
cloud services and an Epic login) is a separate step, see docs/findings/2026-10-10-metahuman-residents.md.
"""
import traceback

import unreal

OUT_DIR = "/Game/Characters/MetaHumans"

# Appearance is a design choice made from each resident's identity text; edit freely and re-run.
RESIDENTS = [
    dict(
        name="MHC_Aster",
        # young, recently awake, gentle, curious; lean, medium height
        body=dict(masculine_feminine=0.25, fat=0.30, muscularity=0.40, height_cm=172.0),
        skin=(0.38, 0.45),
        eyes=(0.75, 0.40),
    ),
    dict(
        name="MHC_Innkeeper",
        # an established host: sturdy, warm, weathered; the inn's keeper, not a young adventurer
        body=dict(masculine_feminine=0.80, fat=0.55, muscularity=0.45, height_cm=166.0),
        skin=(0.30, 0.70),
        eyes=(0.30, 0.35),
    ),
]


def log(msg):
    unreal.log("MetaHumans: " + str(msg))


def make(asset_path):
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        return unreal.load_asset(asset_path)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = unreal.new_object(unreal.MetaHumanCharacterFactoryNew)
    return tools.create_asset(asset_path.rsplit("/", 1)[-1], asset_path.rsplit("/", 1)[0],
                              unreal.MetaHumanCharacter, factory)


def main():
    from metahuman_toolset import metahuman as mh
    from metahuman_toolset.session import get_or_create_session, clear_session_cache

    # Skin/body edits assert inside the engine (BodyTexture) unless the MetaHuman optional content is installed
    # (Epic Games Launcher > Library > UE 5.8 > Options). Without it we still create the character assets so the
    # roster exists, and apply the appearance on a later run.
    have_content = unreal.MetaHumanGeneratorSubsystemWrapper.is_optional_content_installed()
    if not have_content:
        unreal.log_warning("MetaHumans: optional MetaHuman content is not installed; creating asset shells only")
    unreal.EditorAssetLibrary.make_directory(OUT_DIR)
    for r in RESIDENTS:
        path = "{}/{}".format(OUT_DIR, r["name"])
        asset = make(path)
        if not asset:
            raise RuntimeError("could not create " + path)
        if not have_content:
            unreal.EditorAssetLibrary.save_asset(path)
            log("saved shell {} (appearance pending optional content)".format(path))
            continue
        obj_path = asset.get_path_name()
        session = get_or_create_session(obj_path)
        try:
            shape = mh.BodyShape()
            for k, v in r["body"].items():
                setattr(shape, k, v)
            mh.MetaHumanToolset.set_body_shape(session, shape)
            tone = mh.SkinTone()
            tone.lightness, tone.redness = r["skin"]
            mh.MetaHumanToolset.set_skin_tone(session, tone)
            eye = mh.EyeColor()
            eye.temperature, eye.brightness = r["eyes"]
            mh.MetaHumanToolset.set_eye_color(session, eye)
        finally:
            session.finalize()
        unreal.EditorAssetLibrary.save_asset(path)
        log("saved {} body={} skin={} eyes={}".format(path, r["body"], r["skin"], r["eyes"]))
    clear_session_cache()


try:
    main()
except Exception:
    unreal.log_error("MetaHumans FAILED\n" + traceback.format_exc())
