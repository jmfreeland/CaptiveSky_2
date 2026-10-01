"""Creates a separate, weather-responsive opaque water material for the shallow Tideglass pool.

The asset is additive and is never assigned to the saved Island map. Runtime code swaps it onto the
flattened pool surface for Game/PIE only, then restores the authored blockout material at teardown.
This builder refuses to overwrite an existing asset; remove or rename it deliberately to rebuild.
"""

import traceback
import unreal


MATERIAL_PATH = "/Game/Materials/M_TideglassPool"
COLLECTION_PATH = "/Game/Environment/MPC_IslandEnvironment"
WAVE_NORMAL = "/Water/Textures/Normals/T_Water_TilingNormal_Waves_02"
MEL = unreal.MaterialEditingLibrary


def expr(material, cls, x, y, **props):
    node = MEL.create_material_expression(material, cls, x, y)
    if node is None:
        raise RuntimeError("Could not create " + str(cls))
    for key, value in props.items():
        node.set_editor_property(key, value)
    return node


def link(source, destination, destination_pin="", source_pin=""):
    if not MEL.connect_material_expressions(source, source_pin, destination, destination_pin):
        raise RuntimeError("Could not connect {} -> {}.{}".format(
            source.get_class().get_name(), destination.get_class().get_name(), destination_pin))


def binary(material, cls, a, b, x, y, a_pin="", b_pin=""):
    node = expr(material, cls, x, y)
    link(a, node, "A", a_pin)
    link(b, node, "B", b_pin)
    return node


def scalar(material, name, value, x, y):
    return expr(material, unreal.MaterialExpressionScalarParameter, x, y,
                parameter_name=name, default_value=value, group="Tideglass Water")


def vector(material, name, rgb, x, y):
    return expr(material, unreal.MaterialExpressionVectorParameter, x, y,
                parameter_name=name,
                default_value=unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0),
                group="Tideglass Water")


def collection_value(material, collection, name, x, y):
    return expr(material, unreal.MaterialExpressionCollectionParameter, x, y,
                collection=collection, parameter_name=name)


def mask(material, source, x, y, r=False, g=False, b=False):
    node = expr(material, unreal.MaterialExpressionComponentMask, x, y, r=r, g=g, b=b)
    link(source, node)
    return node


def build():
    if unreal.EditorAssetLibrary.does_asset_exist(MATERIAL_PATH):
        raise RuntimeError("Refusing to overwrite existing asset " + MATERIAL_PATH)
    collection = unreal.load_asset(COLLECTION_PATH)
    normal_texture = unreal.load_asset(WAVE_NORMAL)
    if not collection or not normal_texture:
        raise RuntimeError("Missing environment collection or Water plugin normal texture")

    tools = unreal.AssetToolsHelpers.get_asset_tools()
    material = tools.create_asset("M_TideglassPool", "/Game/Materials",
                                  unreal.Material, unreal.MaterialFactoryNew())
    if not material:
        raise RuntimeError("Could not create " + MATERIAL_PATH)

    time = expr(material, unreal.MaterialExpressionTime, -1800, 300)
    world_position = expr(material, unreal.MaterialExpressionWorldPosition, -1800, 0)
    world_xy = mask(material, world_position, -1600, 0, r=True, g=True)

    wind = collection_value(material, collection, "WindSpeed", -1800, -900)
    storm = collection_value(material, collection, "Storm", -1800, -760)
    rain = collection_value(material, collection, "RainIntensity", -1800, -620)
    agitation = expr(material, unreal.MaterialExpressionSaturate, -1100, -720)
    wind_part = binary(material, unreal.MaterialExpressionMultiply, wind,
                       scalar(material, "WindResponse", 0.65, -1600, -900), -1350, -900)
    storm_part = binary(material, unreal.MaterialExpressionMultiply, storm,
                        scalar(material, "StormResponse", 0.75, -1600, -760), -1350, -760)
    rain_part = binary(material, unreal.MaterialExpressionMultiply, rain,
                       scalar(material, "RainResponse", 0.35, -1600, -620), -1350, -620)
    agitation_sum = binary(material, unreal.MaterialExpressionAdd,
                           binary(material, unreal.MaterialExpressionAdd, wind_part, storm_part, -1200, -850),
                           rain_part, -1200, -700)
    link(agitation_sum, agitation)

    # Two slowly scrolling, world-aligned swells keep the broad pool readable without a flat tint.
    def swell(name, tile_cm, velocity, strength, y):
        scaled = binary(material, unreal.MaterialExpressionDivide, world_xy,
                        scalar(material, name + "TileCm", tile_cm, -1600, y + 130), -1350, y)
        drift_vector = expr(material, unreal.MaterialExpressionConstant2Vector, -1600, y + 260,
                            r=velocity[0], g=velocity[1])
        drift = binary(material, unreal.MaterialExpressionMultiply, time, drift_vector, -1350, y + 220)
        uv = binary(material, unreal.MaterialExpressionAdd, scaled, drift, -1120, y)
        sample = expr(material, unreal.MaterialExpressionTextureSample, -900, y,
                      texture=normal_texture, sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        link(uv, sample, "UVs")
        xy = mask(material, sample, -700, y, r=True, g=True)
        weighted = binary(material, unreal.MaterialExpressionMultiply, xy,
                          scalar(material, name + "Strength", strength, -700, y + 150), -500, y)
        return weighted

    swell_a = swell("LongSwell", 5200.0, (0.010, 0.004), 0.32, 0)
    swell_b = swell("ShortChop", 1500.0, (-0.018, 0.014), 0.18, 420)
    slope = binary(material, unreal.MaterialExpressionAdd, swell_a, swell_b, -250, 180)
    weather_gain = binary(material, unreal.MaterialExpressionAdd,
                          scalar(material, "CalmNormalGain", 0.45, -250, 350),
                          binary(material, unreal.MaterialExpressionMultiply, agitation,
                                 scalar(material, "WeatherNormalGain", 0.65, -250, 500), 0, 400),
                          200, 250)
    weather_slope = binary(material, unreal.MaterialExpressionMultiply, slope, weather_gain, 400, 200)
    normal_xy = expr(material, unreal.MaterialExpressionAppendVector, 600, 200)
    link(weather_slope, normal_xy, "A")
    link(expr(material, unreal.MaterialExpressionConstant, 400, 360, r=1.0), normal_xy, "B")
    normalized = expr(material, unreal.MaterialExpressionNormalize, 800, 200)
    link(normal_xy, normalized)
    MEL.connect_material_property(normalized, "", unreal.MaterialProperty.MP_NORMAL)

    pool_color = vector(material, "PoolColor", (0.025, 0.17, 0.22), 300, -250)
    reflection_tint = vector(material, "EdgeReflectionTint", (0.14, 0.43, 0.47), 300, -100)
    fresnel = expr(material, unreal.MaterialExpressionFresnel, 300, 50, exponent=4.0)
    color = expr(material, unreal.MaterialExpressionLinearInterpolate, 700, -180)
    link(pool_color, color, "A")
    link(reflection_tint, color, "B")
    link(fresnel, color, "Alpha")
    MEL.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)

    roughness = binary(material, unreal.MaterialExpressionAdd,
                       scalar(material, "CalmRoughness", 0.18, 700, -600),
                       binary(material, unreal.MaterialExpressionMultiply, agitation,
                              scalar(material, "WeatherRoughness", 0.38, 700, -450), 900, -450),
                       1100, -550)
    MEL.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(scalar(material, "Specular", 0.62, 1000, -300),
                                  "", unreal.MaterialProperty.MP_SPECULAR)

    errors = MEL.recompile_material(material)
    if errors:
        raise RuntimeError("Material compile errors: " + " | ".join(str(error) for error in errors))
    if not unreal.EditorAssetLibrary.save_asset(MATERIAL_PATH):
        raise RuntimeError("Could not save " + MATERIAL_PATH)
    unreal.log("[TideglassMaterial] Created additive weather-responsive pool material {}".format(MATERIAL_PATH))


try:
    build()
except Exception:
    unreal.log_error("[TideglassMaterial] " + traceback.format_exc())
