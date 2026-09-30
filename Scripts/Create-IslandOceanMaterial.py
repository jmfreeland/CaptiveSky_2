"""Builds /Game/Materials/M_IslandOcean, a Single Layer Water material for the Island's ocean plane.

Run in the editor (see Scripts/Capture-Viewpoints.ps1 -OceanMaterial for the preview path). Rebuilds its
own asset on every run and never touches the map or any other asset.

Look: water colour comes from Single Layer Water absorption/scattering, so the shallows over the seabed
read turquoise and deep water darkens. Two scrolling tiling normal maps give the swell; shoreline foam comes
from the gap between the scene depth behind the water and the water surface. Wind, storm and rain read from
/Game/Environment/MPC_IslandEnvironment, so the sea roughens in weather with no code driving it.
"""

import traceback
import unreal

MATERIAL_PATH = "/Game/Materials/M_IslandOcean"
COLLECTION_PATH = "/Game/Environment/MPC_IslandEnvironment"
WAVE_NORMAL = "/Water/Textures/Normals/T_Water_TilingNormal_Waves_02"
FOAM_TEXTURE = "/Water/Textures/Foam/T_WaterFlow_01_Foam_Tiled"

MEL = unreal.MaterialEditingLibrary


def expr(material, cls, x, y, **props):
    node = MEL.create_material_expression(material, cls, x, y)
    if node is None:
        raise RuntimeError("Could not create " + str(cls))
    for key, value in props.items():
        node.set_editor_property(key, value)
    return node


def link(src, dst, dst_pin="", src_pin=""):
    if not MEL.connect_material_expressions(src, src_pin, dst, dst_pin):
        raise RuntimeError("Could not connect {} -> {}.{}".format(src.get_class().get_name(), dst.get_class().get_name(), dst_pin))


def binary(material, cls, a, b, x, y, a_pin="", b_pin=""):
    node = expr(material, cls, x, y)
    link(a, node, "A", a_pin)
    link(b, node, "B", b_pin)
    return node


def scalar(material, name, value, x, y, group="Ocean"):
    return expr(material, unreal.MaterialExpressionScalarParameter, x, y, parameter_name=name, default_value=value, group=group)


def vector3(material, name, rgb, x, y, group="Ocean"):
    return expr(material, unreal.MaterialExpressionVectorParameter, x, y, parameter_name=name,
                default_value=unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0), group=group)


def channels(material, source, x, y, r=False, g=False, b=False, a=False, src_pin=""):
    mask = expr(material, unreal.MaterialExpressionComponentMask, x, y, r=r, g=g, b=b, a=a)
    link(source, mask, "", src_pin)
    return mask


def collection_value(material, collection, name, x, y):
    return expr(material, unreal.MaterialExpressionCollectionParameter, x, y, collection=collection, parameter_name=name)


def build():
    if unreal.EditorAssetLibrary.does_asset_exist(MATERIAL_PATH):
        unreal.EditorAssetLibrary.delete_asset(MATERIAL_PATH)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    material = tools.create_asset("M_IslandOcean", "/Game/Materials", unreal.Material, unreal.MaterialFactoryNew())
    collection = unreal.load_asset(COLLECTION_PATH)
    wave_normal = unreal.load_asset(WAVE_NORMAL)
    foam_texture = unreal.load_asset(FOAM_TEXTURE)
    if not (collection and wave_normal and foam_texture):
        raise RuntimeError("Missing input asset: collection={} normal={} foam={}".format(collection, wave_normal, foam_texture))

    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_SINGLE_LAYER_WATER)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)

    # Weather inputs.
    wind = collection_value(material, collection, "WindSpeed", -1800, -900)
    storm = collection_value(material, collection, "Storm", -1800, -780)
    rain = collection_value(material, collection, "RainIntensity", -1800, -660)
    wind_part = binary(material, unreal.MaterialExpressionMultiply, wind, scalar(material, "WindRoughness", 0.6, -1800, -540), -1500, -900)
    storm_part = binary(material, unreal.MaterialExpressionMultiply, storm, scalar(material, "StormRoughness", 0.6, -1800, -420), -1500, -780)
    rain_part = binary(material, unreal.MaterialExpressionMultiply, rain, scalar(material, "RainRoughness", 0.2, -1800, -300), -1500, -660)
    agitation = expr(material, unreal.MaterialExpressionSaturate, -1000, -800)
    link(binary(material, unreal.MaterialExpressionAdd, binary(material, unreal.MaterialExpressionAdd, wind_part, storm_part, -1300, -850), rain_part, -1150, -800), agitation)

    # Shared coordinates.
    world = expr(material, unreal.MaterialExpressionWorldPosition, -1800, 0)
    world_xy = channels(material, world, -1600, 0, r=True, g=True)
    time = expr(material, unreal.MaterialExpressionTime, -1800, 200)

    # Two swells scrolling in different directions; storm sharpens both.
    def swell(tag, tile_cm, velocity, y):
        scaled = binary(material, unreal.MaterialExpressionDivide, world_xy, scalar(material, tag + "TileCm", tile_cm, -1600, y + 120), -1300, y)
        drift = binary(material, unreal.MaterialExpressionMultiply, time, expr(material, unreal.MaterialExpressionConstant2Vector, -1500, y + 240, r=velocity[0], g=velocity[1]), -1300, y + 200)
        uv = binary(material, unreal.MaterialExpressionAdd, scaled, drift, -1100, y)
        sample = expr(material, unreal.MaterialExpressionTextureSample, -900, y, texture=wave_normal, sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        link(uv, sample, "UVs")
        return channels(material, sample, -700, y, r=True, g=True)

    long_swell = swell("LongSwell", 5200.0, (0.010, 0.004), 0)
    short_chop = swell("ShortChop", 1300.0, (-0.022, 0.016), 380)
    slope = binary(material, unreal.MaterialExpressionAdd,
                   binary(material, unreal.MaterialExpressionMultiply, long_swell, scalar(material, "LongSwellStrength", 0.45, -700, 120), -500, 0),
                   binary(material, unreal.MaterialExpressionMultiply, short_chop, scalar(material, "ShortChopStrength", 0.30, -700, 500), -500, 380), -300, 200)
    roughness_gain = expr(material, unreal.MaterialExpressionLinearInterpolate, -300, -800)
    link(expr(material, unreal.MaterialExpressionConstant, -500, -860, r=1.0), roughness_gain, "A")
    link(scalar(material, "StormSwellGain", 3.0, -500, -740), roughness_gain, "B")
    link(agitation, roughness_gain, "Alpha")
    stormy_slope = binary(material, unreal.MaterialExpressionMultiply, slope, roughness_gain, -100, 200)
    # Far water is flattened: tiled normals at grazing angles read as horizontal streaks.
    distance_fade = expr(material, unreal.MaterialExpressionOneMinus, 100, 120)
    far_ratio = expr(material, unreal.MaterialExpressionSaturate, -100, 120)
    link(binary(material, unreal.MaterialExpressionDivide,
                binary(material, unreal.MaterialExpressionSubtract, expr(material, unreal.MaterialExpressionPixelDepth, -500, 60), scalar(material, "DetailFadeStartCm", 30000.0, -500, 60), -300, 60),
                scalar(material, "DetailFadeRangeCm", 150000.0, -500, 120), -300, 120), far_ratio)
    link(far_ratio, distance_fade)
    stormy_slope = binary(material, unreal.MaterialExpressionMultiply, stormy_slope, distance_fade, 100, 200)
    normal = expr(material, unreal.MaterialExpressionAppendVector, 100, 200)
    link(stormy_slope, normal, "A")
    link(expr(material, unreal.MaterialExpressionConstant, -100, 330, r=1.0), normal, "B")
    normalized = expr(material, unreal.MaterialExpressionNormalize, 300, 200)
    link(normal, normalized)
    MEL.connect_material_property(normalized, "", unreal.MaterialProperty.MP_NORMAL)

    # Shoreline foam from water thickness.
    depth_behind = expr(material, unreal.MaterialExpressionSceneDepthWithoutWater, -1800, 900)
    pixel_depth = expr(material, unreal.MaterialExpressionPixelDepth, -1800, 1000)
    thickness = binary(material, unreal.MaterialExpressionSubtract, depth_behind, pixel_depth, -1500, 950)
    lap = expr(material, unreal.MaterialExpressionSine, -1100, 1150)
    link(binary(material, unreal.MaterialExpressionAdd,
                binary(material, unreal.MaterialExpressionMultiply, time, scalar(material, "FoamLapSpeed", 0.8, -1500, 1150), -1300, 1150),
                binary(material, unreal.MaterialExpressionMultiply, thickness, scalar(material, "FoamLapPhase", 0.012, -1500, 1250), -1300, 1250), -1150, 1200), lap)
    lapping = binary(material, unreal.MaterialExpressionAdd, expr(material, unreal.MaterialExpressionConstant, -900, 1100, r=0.7),
                     binary(material, unreal.MaterialExpressionMultiply, lap, scalar(material, "FoamLapDepth", 0.3, -1100, 1300), -900, 1200), -700, 1150)
    foam_reach = binary(material, unreal.MaterialExpressionMultiply, scalar(material, "FoamReachCm", 70.0, -900, 1000), lapping, -500, 1100)
    edge = expr(material, unreal.MaterialExpressionOneMinus, -100, 1050)
    ratio = expr(material, unreal.MaterialExpressionSaturate, -300, 1050)
    link(binary(material, unreal.MaterialExpressionDivide, thickness, foam_reach, -500, 1000), ratio)
    link(ratio, edge)

    foam_uv = binary(material, unreal.MaterialExpressionAdd,
                     binary(material, unreal.MaterialExpressionDivide, world_xy, scalar(material, "FoamTileCm", 700.0, -1600, 1450), -1300, 1450),
                     binary(material, unreal.MaterialExpressionMultiply, time, expr(material, unreal.MaterialExpressionConstant2Vector, -1500, 1600, r=0.006, g=-0.004), -1300, 1600), -1100, 1500)
    foam_sample = expr(material, unreal.MaterialExpressionTextureSample, -900, 1500, texture=foam_texture)
    link(foam_uv, foam_sample, "UVs")
    foam_noise = channels(material, foam_sample, -700, 1500, r=True)
    one_minus_noise = binary(material, unreal.MaterialExpressionSubtract, expr(material, unreal.MaterialExpressionConstant, -700, 1650, r=1.0), foam_noise, -500, 1650)
    breakup = binary(material, unreal.MaterialExpressionMultiply, one_minus_noise, scalar(material, "FoamBreakup", 0.9, -500, 1750), -300, 1700)
    foam = expr(material, unreal.MaterialExpressionSaturate, 300, 1500)
    link(binary(material, unreal.MaterialExpressionMultiply,
                binary(material, unreal.MaterialExpressionSubtract, edge, breakup, -100, 1500),
                scalar(material, "FoamSharpness", 2.5, -100, 1650), 100, 1500), foam)

    # Surface.
    base_color = binary(material, unreal.MaterialExpressionMultiply, foam, vector3(material, "FoamColor", (0.80, 0.86, 0.88), 100, 1650), 500, 1500)
    MEL.connect_material_property(base_color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    surface_roughness = expr(material, unreal.MaterialExpressionLinearInterpolate, 500, 600)
    calm_to_rough = binary(material, unreal.MaterialExpressionAdd, scalar(material, "CalmRoughness", 0.05, 300, 560),
                           binary(material, unreal.MaterialExpressionMultiply, agitation, scalar(material, "StormRoughnessGain", 0.22, 300, 640), 300, 700), 500, 560)
    link(calm_to_rough, surface_roughness, "A")
    link(scalar(material, "FoamSurfaceRoughness", 0.75, 500, 500), surface_roughness, "B")
    link(foam, surface_roughness, "Alpha")
    MEL.connect_material_property(surface_roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(scalar(material, "Specular", 0.5, 500, 420), "", unreal.MaterialProperty.MP_SPECULAR)

    # Water volume.
    water_out = expr(material, unreal.MaterialExpressionSingleLayerWaterMaterialOutput, 900, 200)
    link(vector3(material, "Scattering", (0.0045, 0.0110, 0.0140), 700, 100), water_out, "ScatteringCoefficients")
    link(vector3(material, "Absorption", (0.0150, 0.0042, 0.0020), 700, 200), water_out, "AbsorptionCoefficients")
    link(scalar(material, "PhaseG", 0.55, 700, 300), water_out, "PhaseG")

    MEL.recompile_material(material)
    unreal.EditorAssetLibrary.save_asset(MATERIAL_PATH)
    unreal.log("[IslandOcean] Built {} (shading model {}).".format(MATERIAL_PATH, material.get_editor_property("shading_model")))


try:
    build()
except Exception:
    unreal.log_error("[IslandOcean] " + traceback.format_exc())
