"""Apply the tested 4K cap to the two Island-referenced landscape textures."""

import json
import os
import traceback

import unreal


TEXTURE_PATHS = (
    "/Game/Fab/Megascans/Surfaces/Desert_Outback_Ground_Dirt_Rocky_09_xisbchvs/Raw/T_xisbchvs_8K_ORM",
    "/Game/Fab/Megascans/Surfaces/Desert_Outback_Ground_Dirt_Rocky_09_xisbchvs/Raw/Desert_Outback_Ground_Dirt_Rocky_09_xisbchvs_8K_Displacement",
)
PARENT_MATERIAL = "/Game/Materials/M_Island_Textured_ShoreSubstrate"


def fail(message):
    raise RuntimeError(message)


def main():
    if os.environ.get("CAPTIVESKY_APPLY_LANDSCAPE_TEXTURE_BUDGET_4K") != "1":
        fail("Set CAPTIVESKY_APPLY_LANDSCAPE_TEXTURE_BUDGET_4K=1 after backing up both source packages.")

    textures = []
    for path in TEXTURE_PATHS:
        texture = unreal.EditorAssetLibrary.load_asset(path)
        if not isinstance(texture, unreal.Texture2D):
            fail("Source Texture2D missing: " + path)
        current_size = int(texture.get_editor_property("max_texture_size"))
        if current_size not in (0, 4096):
            fail("Unexpected pre-existing max_texture_size on {}: {}".format(path, current_size))
        textures.append(texture)

    parent = unreal.EditorAssetLibrary.load_asset(PARENT_MATERIAL)
    if not isinstance(parent, unreal.Material):
        fail("Authored landscape parent could not be loaded: " + PARENT_MATERIAL)

    report = {"textures": [], "material_compile_diagnostics": []}
    for texture in textures:
        texture.set_editor_property("max_texture_size", 4096)
        if not unreal.EditorAssetLibrary.save_loaded_asset(texture):
            fail("Could not save capped texture: " + texture.get_path_name())
        report["textures"].append({
            "asset": texture.get_path_name(),
            "max_texture_size": texture.get_editor_property("max_texture_size"),
            "mip_gen_settings": str(texture.get_editor_property("mip_gen_settings")),
            "compression_settings": str(texture.get_editor_property("compression_settings")),
            "never_stream": texture.get_editor_property("never_stream"),
        })

    diagnostics = unreal.MaterialEditingLibrary.recompile_material(parent)
    if diagnostics:
        fail("Authored landscape parent compile diagnostics after texture cap: {}".format(diagnostics))
    report["material_compile_diagnostics"] = list(diagnostics)
    unreal.log("[LandscapeTextureBudget4KApply] REPORT " + json.dumps(report, sort_keys=True))
    unreal.log("[LandscapeTextureBudget4KApply] COMPLETE textures={}".format(len(report["textures"])))


try:
    main()
except Exception:
    unreal.log_error("[LandscapeTextureBudget4KApply] FAILED " + traceback.format_exc())
    raise
