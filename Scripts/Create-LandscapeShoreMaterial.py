"""Builds a shoreline variant of the Island landscape material: a sand layer where the land meets the sea.

The island's auto-material has rocky ground, mossy grass, rock, cliff, snow and road layers but nothing for the shore, so the
beach is just ground texture running into the water (and the sea floor in the shallows is the same ground texture, which the Water
plugin shows through). This adds one tileable generated texture set (T_Lib_ShoreSand_{BC,N,ORM}, made by the ComfyUI asset
pipeline, see Scripts/AssetPipeline) as a new layer.

Creates, from the existing wet graph (so rain still darkens the sand):
  /Game/Materials/M_Island_Textured_Shore      copy of M_Island_Textured_Wet with a sand stage inserted before its wet stage
  /Game/Materials/MI_Island_Landscape_Shore    copy of MI_Island_Landscape_Wet reparented to it (all authored layer overrides kept)

Sand stage (all in world space, every number a material parameter in group "Shore", so tune it on the instance, no rebuild):
  - height mask: sand from the sea floor up to ShoreTopCm above SeaLevelCm, soft over ShoreFadeCm, edge broken up by tiled noise
    so the waterline is organic (ShoreBreakupCm);
  - slope mask: only on gentle ground (vertex normal Z above ShoreSlopeMin), so steep rocky shore keeps its cliff/rock layers;
  - two-scale sampling (SandTileCm and a larger macro tile) so the sand does not visibly repeat;
  - damp band: sand within SandWetBandCm of the waterline is permanently darker and a little glossier.
Needs the sand PNGs from the pipeline's export folder; they are imported to /Game/Generated/ComfyBlender/Library/.

Authored assets are NOT touched unless LANDSCAPE_SHORE_AUTHOR=1: then /Game/Materials/MI_Island_Landscape (the map's landscape
material) is reparented onto the shore graph, after copying it once to MI_Island_Landscape_PreShoreBackup. LANDSCAPE_SHORE_AUTHOR=0
puts the previous parent back.

Run in a headless editor on a scratch project that junctions Content (the main editor may be open):
  UnrealEditor-Cmd.exe <scratch .uproject> -ExecutePythonScript=<path without spaces> -unattended -nosplash -nosound -RenderOffscreen
      -NoZen "-LocalDataCachePath=..." "-shaderworkingdir=..." "-abslog=..."
Preview: Scripts/Capture-Viewpoints.ps1 -Hour 12 -Only ShoreApproach -LandscapeParent /Game/Materials/M_Island_Textured_Shore
"""

import os
import traceback

import unreal

TAG = "[LandscapeShore] "
BASE_PARENT = "/Game/Materials/M_Island_Textured_Wet"
BASE_INSTANCE = "/Game/Materials/MI_Island_Landscape_Wet"
ORIGINAL_PARENT = "/Game/Materials/M_Island_Textured_Auto"
PARENT_OUT = "/Game/Materials/M_Island_Textured_Shore"
INSTANCE_OUT = "/Game/Materials/MI_Island_Landscape_Shore"
AUTHORED = "/Game/Materials/MI_Island_Landscape"
AUTHORED_BACKUP = "/Game/Materials/MI_Island_Landscape_PreShoreBackup"
NOISE_TEXTURE = "/Game/Materals/Textures/T_LandscapeNoise"
TEX_DEST = "/Game/Generated/ComfyBlender/Library"
SAND_NAME = "T_Lib_ShoreSand"
GROUP = "Shore"
SEA_LEVEL_CM = 940.0          # WaterBodyOcean Z (Scripts/Create-IslandWaterBody.py)

MEL = unreal.MaterialEditingLibrary


def log(msg):
    unreal.log(TAG + msg)


# ---------------------------------------------------------------------------------------------------------------- graph helpers
def expr(material, cls, x, y, **props):
    node = MEL.create_material_expression(material, cls, x, y)
    if node is None:
        raise RuntimeError("Could not create " + str(cls))
    for key, value in props.items():
        node.set_editor_property(key, value)
    return node


def link(src, dst, dst_pin="", src_pin=""):
    """src may be a node or a (node, output pin name) tuple, e.g. one output of a Break node."""
    if isinstance(src, tuple):
        src, src_pin = src
    if not MEL.connect_material_expressions(src, src_pin, dst, dst_pin):
        raise RuntimeError("Could not connect {}.{} -> {}.{}".format(src.get_class().get_name(), src_pin, dst.get_class().get_name(), dst_pin))


def binary(material, cls, a, b, x, y):
    node = expr(material, cls, x, y)
    link(a, node, "A")
    link(b, node, "B")
    return node


def mul(m, a, b, x, y):
    return binary(m, unreal.MaterialExpressionMultiply, a, b, x, y)


def add(m, a, b, x, y):
    return binary(m, unreal.MaterialExpressionAdd, a, b, x, y)


def sub(m, a, b, x, y):
    return binary(m, unreal.MaterialExpressionSubtract, a, b, x, y)


def div(m, a, b, x, y):
    return binary(m, unreal.MaterialExpressionDivide, a, b, x, y)


def lerp(m, a, b, alpha, x, y):
    node = expr(m, unreal.MaterialExpressionLinearInterpolate, x, y)
    link(a, node, "A")
    link(b, node, "B")
    link(alpha, node, "Alpha")
    return node


def saturate(m, source, x, y):
    node = expr(m, unreal.MaterialExpressionSaturate, x, y)
    link(source, node)
    return node


def constant(m, value, x, y):
    return expr(m, unreal.MaterialExpressionConstant, x, y, r=value)


def scalar(m, name, value, x, y):
    return expr(m, unreal.MaterialExpressionScalarParameter, x, y, parameter_name=name, default_value=value, group=GROUP)


def channel(m, source, x, y, r=False, g=False, b=False):
    mask = expr(m, unreal.MaterialExpressionComponentMask, x, y, r=r, g=g, b=b)
    link(source, mask)
    return mask


def tex_param(m, name, texture, sampler, x, y):
    return expr(m, unreal.MaterialExpressionTextureSampleParameter2D, x, y, parameter_name=name, texture=texture,
                sampler_type=sampler, group=GROUP, sampler_source=unreal.SamplerSourceMode.SSM_WRAP_WORLD_GROUP_SETTINGS)


def delete_if_exists(path):
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        if not unreal.EditorAssetLibrary.delete_asset(path):
            raise RuntimeError("Could not delete " + path)


# ---------------------------------------------------------------------------------------------------------------- textures
def pipeline_texture_dir():
    here = os.path.dirname(os.path.abspath(__file__))
    repo = os.path.abspath(os.path.join(here, ".."))
    return os.environ.get("ASSET_EXPORT_ROOT", os.path.join(repo, "Saved", "CaptiveSky", "ComfyBlender")).replace("\\", "/") + "/Library/Textures"


def import_texture(suffix, srgb, compression):
    name = "{}_{}".format(SAND_NAME, suffix)
    path = "{}/{}.{}".format(TEX_DEST, name, name)
    source = "{}/{}.png".format(pipeline_texture_dir(), name)
    if not os.path.exists(source):
        raise RuntimeError("Missing pipeline texture " + source)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", source)
    task.set_editor_property("destination_path", TEX_DEST)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", True)
    task.set_editor_property("save", True)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.load_asset(path)
    if not texture:
        raise RuntimeError("Texture import failed: " + path)
    texture.set_editor_property("srgb", srgb)
    if compression is not None:
        texture.set_editor_property("compression_settings", compression)
    texture.set_editor_property("address_x", unreal.TextureAddress.TA_WRAP)
    texture.set_editor_property("address_y", unreal.TextureAddress.TA_WRAP)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)
    log("imported {} ({}x{})".format(path, texture.blueprint_get_size_x(), texture.blueprint_get_size_y()))
    return texture


# ---------------------------------------------------------------------------------------------------------------- build
def build():
    unreal.EditorAssetLibrary.make_directory(TEX_DEST)
    base_c = import_texture("BC", True, None)
    normal = import_texture("N", False, unreal.TextureCompressionSettings.TC_NORMALMAP)
    orm = import_texture("ORM", False, unreal.TextureCompressionSettings.TC_MASKS)
    noise = unreal.load_asset(NOISE_TEXTURE)
    if not noise:
        raise RuntimeError("Missing noise texture " + NOISE_TEXTURE)

    authored = unreal.load_asset(AUTHORED)
    base_instance = unreal.load_asset(BASE_INSTANCE)
    original = unreal.load_asset(ORIGINAL_PARENT)
    if not (authored and base_instance and original):
        raise RuntimeError("Missing a source asset (authored / wet instance / original parent); run Create-LandscapeWetMaterial.py first")
    mode = os.environ.get("LANDSCAPE_SHORE_AUTHOR", "")
    authored_was_shore = PARENT_OUT.rsplit("/", 1)[-1] in authored.get_editor_property("parent").get_path_name()
    if authored_was_shore:  # about to delete the parent it uses: detach first
        authored.set_editor_property("parent", unreal.load_asset(BASE_PARENT))
        MEL.update_material_instance(authored)
        unreal.EditorAssetLibrary.save_asset(AUTHORED)
    elif mode == "1" and not unreal.EditorAssetLibrary.does_asset_exist(AUTHORED_BACKUP):
        if not unreal.EditorAssetLibrary.duplicate_asset(AUTHORED, AUTHORED_BACKUP):
            raise RuntimeError("Could not back up " + AUTHORED)
        unreal.EditorAssetLibrary.save_asset(AUTHORED_BACKUP)

    delete_if_exists(INSTANCE_OUT)
    delete_if_exists(PARENT_OUT)
    m = unreal.EditorAssetLibrary.duplicate_asset(BASE_PARENT, PARENT_OUT)
    if not m:
        raise RuntimeError("Could not duplicate " + BASE_PARENT)

    # Locate the wet stage's Break node (the one fed by the original attributes) and the original source pin.
    final_make = MEL.get_material_property_input_node(m, unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
    if final_make is None:
        raise RuntimeError("Wet parent has nothing on its attributes output")
    wet_break = next((n for n in MEL.get_inputs_for_material_expression(m, final_make)
                      if n.get_class().get_name() == "MaterialExpressionBreakMaterialAttributes"), None)
    if wet_break is None:
        raise RuntimeError("Could not find the wet stage's BreakMaterialAttributes; has Create-LandscapeWetMaterial.py changed?")
    sources = MEL.get_inputs_for_material_expression(m, wet_break)
    if not sources:
        raise RuntimeError("Wet stage's Break has no input")
    source = sources[0]
    source_pin = MEL.get_material_property_input_node_output_name(original, unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
    log("original attributes come from {} (pin '{}')".format(source.get_class().get_name(), source_pin))

    X0, Y0 = 7000, -1200
    brk = expr(m, unreal.MaterialExpressionBreakMaterialAttributes, X0, Y0)
    link(source, brk, "", source_pin)

    world = expr(m, unreal.MaterialExpressionWorldPosition, X0, Y0 + 400)
    wxy = channel(m, world, X0 + 200, Y0 + 400, r=True, g=True)
    wz = channel(m, world, X0 + 200, Y0 + 520, b=True)

    # --- height mask with a noisy edge
    sea = scalar(m, "SeaLevelCm", SEA_LEVEL_CM, X0 + 200, Y0 + 640)
    rel_h = sub(m, wz, sea, X0 + 400, Y0 + 560)
    noise_uv = div(m, wxy, scalar(m, "ShoreBreakupTileCm", 900.0, X0 + 400, Y0 + 700), X0 + 600, Y0 + 700)
    noise_s = expr(m, unreal.MaterialExpressionTextureSample, X0 + 800, Y0 + 700, texture=noise,
                   sampler_source=unreal.SamplerSourceMode.SSM_WRAP_WORLD_GROUP_SETTINGS)
    link(noise_uv, noise_s, "UVs")
    noise_v = channel(m, noise_s, X0 + 1000, Y0 + 700, r=True)
    wobble = mul(m, sub(m, noise_v, constant(m, 0.5, X0 + 1000, Y0 + 800), X0 + 1200, Y0 + 700),
                 mul(m, constant(m, 2.0, X0 + 1000, Y0 + 900), scalar(m, "ShoreBreakupCm", 60.0, X0 + 1000, Y0 + 960), X0 + 1200, Y0 + 900), X0 + 1400, Y0 + 760)
    h = add(m, rel_h, wobble, X0 + 1600, Y0 + 640)
    top = scalar(m, "ShoreTopCm", 170.0, X0 + 1600, Y0 + 760)
    fade = scalar(m, "ShoreFadeCm", 90.0, X0 + 1600, Y0 + 860)
    sand_h = saturate(m, div(m, sub(m, top, h, X0 + 1800, Y0 + 700), fade, X0 + 2000, Y0 + 700), X0 + 2200, Y0 + 700)

    # --- slope mask: gentle ground only
    nz = channel(m, expr(m, unreal.MaterialExpressionVertexNormalWS, X0 + 1600, Y0 + 1100), X0 + 1800, Y0 + 1100, b=True)
    flat = saturate(m, div(m, sub(m, nz, scalar(m, "ShoreSlopeMin", 0.78, X0 + 1800, Y0 + 1200), X0 + 2000, Y0 + 1100),
                           scalar(m, "ShoreSlopeFade", 0.12, X0 + 2000, Y0 + 1200), X0 + 2200, Y0 + 1100), X0 + 2400, Y0 + 1100)
    mask = mul(m, sand_h, flat, X0 + 2600, Y0 + 900)

    # --- sand sampling: detail tile + larger macro tile at an offset, blended
    tile_a = div(m, wxy, scalar(m, "SandTileCm", 350.0, X0 + 400, Y0 + 1400), X0 + 600, Y0 + 1400)
    off = expr(m, unreal.MaterialExpressionConstant2Vector, X0 + 400, Y0 + 1600, r=0.31, g=0.57)
    tile_b = add(m, div(m, wxy, scalar(m, "SandMacroTileCm", 1700.0, X0 + 400, Y0 + 1700), X0 + 600, Y0 + 1700), off, X0 + 800, Y0 + 1700)
    bc_tex = unreal.load_asset("{}/{}_BC.{}_BC".format(TEX_DEST, SAND_NAME, SAND_NAME))
    n_tex = unreal.load_asset("{}/{}_N.{}_N".format(TEX_DEST, SAND_NAME, SAND_NAME))
    orm_tex = unreal.load_asset("{}/{}_ORM.{}_ORM".format(TEX_DEST, SAND_NAME, SAND_NAME))
    S = unreal.MaterialSamplerType
    bc_a = tex_param(m, "Shore Sand Base Color", bc_tex, S.SAMPLERTYPE_COLOR, X0 + 1000, Y0 + 1400)
    link(tile_a, bc_a, "UVs")
    bc_b = expr(m, unreal.MaterialExpressionTextureSample, X0 + 1000, Y0 + 1650, texture=bc_tex, sampler_type=S.SAMPLERTYPE_COLOR,
                sampler_source=unreal.SamplerSourceMode.SSM_WRAP_WORLD_GROUP_SETTINGS)
    link(tile_b, bc_b, "UVs")
    macro = scalar(m, "SandMacroMix", 0.4, X0 + 1000, Y0 + 1850)
    sand_color = lerp(m, bc_a, bc_b, macro, X0 + 1300, Y0 + 1500)

    n_a = tex_param(m, "Shore Sand Normal", n_tex, S.SAMPLERTYPE_NORMAL, X0 + 1000, Y0 + 2000)
    link(tile_a, n_a, "UVs")
    flat_n = expr(m, unreal.MaterialExpressionConstant3Vector, X0 + 1000, Y0 + 2200, constant=unreal.LinearColor(0.0, 0.0, 1.0, 1.0))
    sand_n = lerp(m, flat_n, n_a, scalar(m, "SandNormalIntensity", 1.0, X0 + 1000, Y0 + 2300), X0 + 1300, Y0 + 2100)

    orm_a = tex_param(m, "Shore Sand ORM", orm_tex, S.SAMPLERTYPE_MASKS, X0 + 1000, Y0 + 2500)
    link(tile_a, orm_a, "UVs")
    sand_ao = channel(m, orm_a, X0 + 1300, Y0 + 2450, r=True)
    sand_rough = channel(m, orm_a, X0 + 1300, Y0 + 2600, g=True)

    # --- permanent damp band along the waterline
    band = scalar(m, "SandWetBandCm", 45.0, X0 + 1600, Y0 + 2600)
    damp = saturate(m, div(m, sub(m, band, rel_h, X0 + 1800, Y0 + 2500), band, X0 + 2000, Y0 + 2500), X0 + 2200, Y0 + 2500)
    damp_dark = lerp(m, constant(m, 1.0, X0 + 2200, Y0 + 2600), scalar(m, "SandDampDarken", 0.55, X0 + 2200, Y0 + 2680), damp, X0 + 2400, Y0 + 2560)
    damp_gloss = lerp(m, constant(m, 1.0, X0 + 2200, Y0 + 2800), constant(m, 0.65, X0 + 2200, Y0 + 2860), damp, X0 + 2400, Y0 + 2800)
    wet_sand_color = mul(m, sand_color, damp_dark, X0 + 2600, Y0 + 1600)
    wet_sand_rough = mul(m, sand_rough, damp_gloss, X0 + 2600, Y0 + 2600)

    # --- blend over the original attributes
    make = expr(m, unreal.MaterialExpressionMakeMaterialAttributes, X0 + 3400, Y0)
    link(lerp(m, (brk, "BaseColor"), wet_sand_color, mask, X0 + 3000, Y0 + 200), make, "BaseColor")
    link(lerp(m, (brk, "Roughness"), wet_sand_rough, mask, X0 + 3000, Y0 + 500), make, "Roughness")
    link(lerp(m, (brk, "Normal"), sand_n, mask, X0 + 3000, Y0 + 800), make, "Normal")
    link(lerp(m, (brk, "Metallic"), constant(m, 0.0, X0 + 3000, Y0 + 1000), mask, X0 + 3200, Y0 + 1000), make, "Metallic")
    link(lerp(m, (brk, "Specular"), constant(m, 0.5, X0 + 3000, Y0 + 1200), mask, X0 + 3200, Y0 + 1200), make, "Specular")
    link(lerp(m, (brk, "AmbientOcclusion"), sand_ao, mask, X0 + 3000, Y0 + 1400), make, "AmbientOcclusion")

    # Feed the wet stage from the sand stage instead of from the original attributes.
    link(make, wet_break, "")
    MEL.recompile_material(m)
    if not unreal.EditorAssetLibrary.save_asset(PARENT_OUT):
        raise RuntimeError("Could not save " + PARENT_OUT)

    instance = unreal.EditorAssetLibrary.duplicate_asset(BASE_INSTANCE, INSTANCE_OUT)
    if not instance:
        raise RuntimeError("Could not duplicate " + BASE_INSTANCE)
    instance.set_editor_property("parent", m)
    MEL.update_material_instance(instance)
    if not unreal.EditorAssetLibrary.save_asset(INSTANCE_OUT):
        raise RuntimeError("Could not save " + INSTANCE_OUT)

    if mode == "1" or (authored_was_shore and mode != "0"):
        authored.set_editor_property("parent", m)
        MEL.update_material_instance(authored)
        unreal.EditorAssetLibrary.save_asset(AUTHORED)
    parent_now = authored.get_editor_property("parent").get_path_name()
    log("built {} -> {}; {} now uses {}".format(INSTANCE_OUT, instance.get_editor_property("parent").get_path_name(), AUTHORED, parent_now))
    if mode not in ("1",) and not authored_was_shore and PARENT_OUT.rsplit("/", 1)[-1] in parent_now:
        raise RuntimeError("Authored instance unexpectedly uses the shore parent")


try:
    build()
    log("DONE")
except Exception:
    unreal.log_error(TAG + traceback.format_exc())
