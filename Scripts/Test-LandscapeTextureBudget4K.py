"""Create a disposable 4K landscape-material variant without touching sources."""

import json
import traceback

import unreal


TRIAL_ROOT = "/Game/Generated/TextureBudgetTrial_20261012"
TEXTURE_FOLDER = TRIAL_ROOT + "/Textures"
PARENT_PATH = "/Game/Materials/M_Island_Textured_ShoreSubstrate"
TRIAL_PARENT_PATH = TRIAL_ROOT + "/M_Island_Textured_ShoreSubstrate_4K"
TEXTURES = (
    (
        "/Game/Fab/Megascans/Surfaces/Desert_Outback_Ground_Dirt_Rocky_09_xisbchvs/Raw/T_xisbchvs_8K_ORM",
        TEXTURE_FOLDER + "/T_xisbchvs_ORM_4K",
    ),
    (
        "/Game/Fab/Megascans/Surfaces/Desert_Outback_Ground_Dirt_Rocky_09_xisbchvs/Raw/Desert_Outback_Ground_Dirt_Rocky_09_xisbchvs_8K_Displacement",
        TEXTURE_FOLDER + "/T_xisbchvs_Displacement_4K",
    ),
)


def fail(message):
    raise RuntimeError(message)


def main():
    if unreal.EditorAssetLibrary.does_directory_exist(TRIAL_ROOT):
        fail("Refusing to reuse existing trial folder: " + TRIAL_ROOT)

    unreal.EditorAssetLibrary.make_directory(TEXTURE_FOLDER)
    replacements = {}
    report = {"trial_root": TRIAL_ROOT, "textures": [], "material": None}
    for source_path, destination_path in TEXTURES:
        source = unreal.EditorAssetLibrary.load_asset(source_path)
        if not isinstance(source, unreal.Texture2D):
            fail("Source Texture2D missing: " + source_path)
        trial = unreal.EditorAssetLibrary.duplicate_asset(source_path, destination_path)
        if not isinstance(trial, unreal.Texture2D):
            fail("Could not duplicate source texture: " + source_path)
        trial.set_editor_property("max_texture_size", 4096)
        if not unreal.EditorAssetLibrary.save_loaded_asset(trial):
            fail("Could not save trial texture: " + destination_path)
        replacements[source.get_path_name()] = trial
        report["textures"].append({
            "source": source.get_path_name(),
            "trial": trial.get_path_name(),
            "max_texture_size": trial.get_editor_property("max_texture_size"),
            "mip_gen_settings": str(trial.get_editor_property("mip_gen_settings")),
            "compression_settings": str(trial.get_editor_property("compression_settings")),
            "never_stream": trial.get_editor_property("never_stream"),
        })

    source_material = unreal.EditorAssetLibrary.load_asset(PARENT_PATH)
    if not isinstance(source_material, unreal.Material):
        fail("Landscape source material missing: " + PARENT_PATH)
    trial_material = unreal.EditorAssetLibrary.duplicate_asset(PARENT_PATH, TRIAL_PARENT_PATH)
    if not isinstance(trial_material, unreal.Material):
        fail("Could not duplicate landscape material: " + PARENT_PATH)

    replacement_counts = {path: 0 for path in replacements}
    for expression in unreal.MaterialEditingLibrary.get_material_expressions(trial_material):
        if not isinstance(expression, unreal.MaterialExpressionTextureSample):
            continue
        texture = expression.get_editor_property("texture")
        if not texture:
            continue
        replacement = replacements.get(texture.get_path_name())
        if replacement:
            expression.set_editor_property("texture", replacement)
            replacement_counts[texture.get_path_name()] += 1

    if any(count == 0 for count in replacement_counts.values()):
        fail("The landscape material did not expose all expected texture samples: {}".format(
            replacement_counts))
    diagnostics = unreal.MaterialEditingLibrary.recompile_material(trial_material)
    if diagnostics:
        fail("4K material recompile diagnostics: {}".format(diagnostics))
    if not unreal.EditorAssetLibrary.save_loaded_asset(trial_material):
        fail("Could not save trial landscape material: " + TRIAL_PARENT_PATH)

    report["material"] = {
        "source": source_material.get_path_name(),
        "trial": trial_material.get_path_name(),
        "replacement_counts": replacement_counts,
        "compile_diagnostics": list(diagnostics),
    }
    unreal.log("[LandscapeTextureBudget4K] REPORT " + json.dumps(report, sort_keys=True))
    unreal.log("[LandscapeTextureBudget4K] COMPLETE replacements={}".format(
        sum(replacement_counts.values())))


try:
    main()
except Exception:
    unreal.log_error("[LandscapeTextureBudget4K] FAILED " + traceback.format_exc())
    raise
