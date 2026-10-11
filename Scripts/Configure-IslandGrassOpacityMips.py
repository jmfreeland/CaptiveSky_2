"""Apply the backed-up, 4K mipmapped Island grass-opacity trial in UE 5.8.

Run once with UnrealEditor-Cmd.exe while no interactive editor is open:
  UnrealEditor-Cmd.exe CaptiveSky_2.uproject -ExecutePythonScript="Scripts/Configure-IslandGrassOpacityMips.py" -unattended -NoZen

This changes one local Fab texture asset. The caller must first create and hash
the Saved/Backups/springGrass_Opacity_8k_PreMips_20261011.uasset restore point.
The guard refuses to overwrite an asset that no longer has the audited source
settings (8K, no mips, max size 0, streamable).
"""

import traceback

import unreal


TEXTURE_PATH = "/Game/PN_FoliageCollection/Textures/grassTextures/MasterTextures/springGrass_Opacity_8k"
MAX_TEXTURE_SIZE = 4096


def main():
    texture = unreal.EditorAssetLibrary.load_asset(TEXTURE_PATH)
    if not texture or not isinstance(texture, unreal.Texture2D):
        raise RuntimeError("Expected Texture2D not found: {}".format(TEXTURE_PATH))

    current_mips = texture.get_editor_property("mip_gen_settings")
    current_max_size = int(texture.get_editor_property("max_texture_size"))
    current_never_stream = bool(texture.get_editor_property("never_stream"))
    expected_mips = unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS
    if (current_mips != expected_mips or current_max_size != 0 or current_never_stream):
        raise RuntimeError(
            "Texture settings changed since audit; refusing to overwrite: "
            "mip_gen_settings={} max_texture_size={} never_stream={}".format(
                current_mips, current_max_size, current_never_stream))

    target_mips = unreal.TextureMipGenSettings.TMGS_FROM_TEXTURE_GROUP
    texture.set_editor_property("mip_gen_settings", target_mips)
    texture.set_editor_property("max_texture_size", MAX_TEXTURE_SIZE)
    if not unreal.EditorAssetLibrary.save_asset(TEXTURE_PATH, only_if_is_dirty=False):
        raise RuntimeError("Failed to save texture package: {}".format(TEXTURE_PATH))

    unreal.log("[GrassOpacityMipTrial] saved={} mip_gen_settings={} max_texture_size={} never_stream={}".format(
        TEXTURE_PATH,
        texture.get_editor_property("mip_gen_settings"),
        texture.get_editor_property("max_texture_size"),
        texture.get_editor_property("never_stream")))


try:
    main()
except Exception:
    unreal.log_error("[GrassOpacityMipTrial] FAILED " + traceback.format_exc())
    raise
