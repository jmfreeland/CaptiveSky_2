"""Creates a separate, visibly wave-shaped opaque water material for the Tideglass pool.

The asset is additive and is never assigned to the saved Island map. Runtime code swaps it onto the
procedural pool surface for Game/PIE only, then restores the authored blockout at teardown. Tighter
world-aligned normal tiling makes waves legible at this small pool scale; weather also changes base
color, roughness and normal strength. This output uses a new path and the builder refuses to overwrite
it; the previous M_TideglassPool_NormalizedWind asset is preserved.
"""

import traceback
import os
import unreal


MATERIAL_NAME = os.environ.get("CAPTIVESKY_TIDEGLASS_MATERIAL_NAME", "M_TideglassPool_Lively")
MATERIAL_PATH = "/Game/Materials/" + MATERIAL_NAME
IS_GRAZING_READABLE_CANDIDATE = MATERIAL_NAME == "M_TideglassPool_GrazingReadable"
IS_RIPPLE_VARIANT = MATERIAL_NAME in ("M_TideglassPool_Ripple", "M_TideglassPool_RippleWake")
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


def unary(material, cls, source, x, y):
    node = expr(material, cls, x, y)
    link(source, node)
    return node


def scalar(material, name, value, x, y):
    return expr(material, unreal.MaterialExpressionScalarParameter, x, y,
                parameter_name=name, default_value=value, group="Tideglass Water")


def vector(material, name, rgb, x, y):
    return expr(material, unreal.MaterialExpressionVectorParameter, x, y,
                parameter_name=name,
                default_value=unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0),
                group="Tideglass Water")


def constant(material, value, x, y):
    return expr(material, unreal.MaterialExpressionConstant, x, y, r=value)


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
    material = tools.create_asset(MATERIAL_NAME, "/Game/Materials",
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
    # MPC WindSpeed is in cm/s. Normalize before applying the response so ordinary breeze
    # does not immediately saturate the material's weather blend.
    wind_fraction = binary(material, unreal.MaterialExpressionDivide, wind,
                           scalar(material, "WindNormalizationCmPerSec", 300.0, -1600, -980), -1450, -930)
    wind_fraction_clamped = expr(material, unreal.MaterialExpressionSaturate, -1250, -930)
    link(wind_fraction, wind_fraction_clamped)
    wind_part = binary(material, unreal.MaterialExpressionMultiply, wind_fraction_clamped,
                       scalar(material, "WindResponse", 0.65, -1600, -820), -1050, -900)
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

    # The pool is only a few metres across: ocean-scale UV tiling made the whole surface
    # sample one nearly constant part of the normal texture. These wavelengths span it.
    swell_a = swell("LongSwell", 420.0, (0.035, 0.012), 0.40, 0)
    swell_b = swell("ShortChop", 160.0, (-0.075, 0.058), 0.26, 420)
    slope = binary(material, unreal.MaterialExpressionAdd, swell_a, swell_b, -250, 180)
    weather_gain = binary(material, unreal.MaterialExpressionAdd,
                          scalar(material, "CalmNormalGain", 0.65, -250, 350),
                          binary(material, unreal.MaterialExpressionMultiply, agitation,
                                 scalar(material, "WeatherNormalGain", 1.4, -250, 500), 0, 400),
                          200, 250)
    weather_slope = binary(material, unreal.MaterialExpressionMultiply, slope, weather_gain, 400, 200)
    final_slope = weather_slope

    if IS_RIPPLE_VARIANT:
        # A short-lived radial normal front makes surface breaks deform the water's own
        # reflections. Runtime code drives the world-space origin and impulse age; the
        # fallback grazing-readable material remains unchanged if this asset is absent.
        ripple_center = vector(material, "RippleCenter", (0.0, 0.0, 0.0), -1800, 1120)
        ripple_center_xy = mask(material, ripple_center, -1600, 1120, r=True, g=True)
        ripple_offset = binary(material, unreal.MaterialExpressionSubtract,
                               world_xy, ripple_center_xy, -1400, 1120)
        ripple_distance = binary(material, unreal.MaterialExpressionDistance,
                                 world_xy, ripple_center_xy, -1200, 980)
        ripple_age = binary(material, unreal.MaterialExpressionMax,
                            binary(material, unreal.MaterialExpressionSubtract,
                                   time, scalar(material, "RippleStartTime", 0.0, -1600, 1320),
                                   -1400, 1320),
                            constant(material, 0.0, -1400, 1420), -1200, 1280)
        ripple_front_radius = binary(material, unreal.MaterialExpressionMultiply,
                                      ripple_age,
                                      scalar(material, "RippleSpeedCmPerSecond", 64.0, -1200, 1480),
                                      -1000, 1280)
        front_width = scalar(material, "RippleWidthCm", 16.0, -800, 1280)

        def softened_ring_profile(ring_radius, y):
            distance_from_ring = unary(material, unreal.MaterialExpressionAbs,
                                        binary(material, unreal.MaterialExpressionSubtract,
                                               ripple_distance, ring_radius, -800, y),
                                        -600, y)
            ring_fraction = unary(material, unreal.MaterialExpressionSaturate,
                                  binary(material, unreal.MaterialExpressionDivide,
                                         distance_from_ring, front_width, -400, y),
                                  -200, y)
            ring_profile = binary(material, unreal.MaterialExpressionSubtract,
                                  constant(material, 1.0, -200, y + 80),
                                  ring_fraction, 0, y)
            softened = expr(material, unreal.MaterialExpressionPower, 200, y)
            link(ring_profile, softened, "Base")
            link(constant(material, 2.0, 0, y + 80), softened, "Exp")
            return softened

        # A leading ring alone reads as a graphic outline. Leave two weaker,
        # expanding crests behind it so the pool itself shows the passing wave train.
        ripple_train = softened_ring_profile(ripple_front_radius, 1120)
        ripple_spacing = scalar(material, "RippleSpacingCm", 24.0, -1000, 1600)
        for trail_index, trail_weight, y in ((1, 0.38, 1720), (2, 0.18, 2020)):
            trail_offset = binary(material, unreal.MaterialExpressionMultiply,
                                  ripple_spacing, constant(material, float(trail_index), -1000, y + 100),
                                  -800, y + 100)
            trail_radius = binary(material, unreal.MaterialExpressionSubtract,
                                  ripple_front_radius, trail_offset, -600, y)
            trail_radius_active = unary(material, unreal.MaterialExpressionSaturate,
                                        binary(material, unreal.MaterialExpressionDivide,
                                               trail_radius, ripple_spacing, -400, y + 120),
                                        -200, y + 120)
            safe_trail_radius = binary(material, unreal.MaterialExpressionMax,
                                       trail_radius, constant(material, 0.0, -400, y + 220),
                                       -200, y + 40)
            trail_profile = softened_ring_profile(safe_trail_radius, y + 320)
            gated_profile = binary(material, unreal.MaterialExpressionMultiply,
                                    trail_profile, trail_radius_active, 0, y + 320)
            weighted_profile = binary(material, unreal.MaterialExpressionMultiply,
                                      gated_profile, constant(material, trail_weight, 200, y + 420),
                                      200, y + 320)
            ripple_train = binary(material, unreal.MaterialExpressionAdd,
                                  ripple_train, weighted_profile, 400, y + 300)
        ripple_age_fraction = unary(material, unreal.MaterialExpressionSaturate,
                                    binary(material, unreal.MaterialExpressionDivide,
                                           ripple_age,
                                           scalar(material, "RippleDurationSeconds", 1.15, -400, 1440),
                                           -200, 1440),
                                    0, 1440)
        ripple_fade = binary(material, unreal.MaterialExpressionSubtract,
                             constant(material, 1.0, 200, 1480), ripple_age_fraction, 400, 1440)
        ripple_envelope = binary(material, unreal.MaterialExpressionMultiply,
                                 ripple_train, ripple_fade, 600, 1260)
        ripple_strength = binary(material, unreal.MaterialExpressionMultiply,
                                 ripple_envelope,
                                 scalar(material, "RippleAmplitude", 0.0, 400, 1420),
                                 800, 1260)
        ripple_direction = expr(material, unreal.MaterialExpressionNormalize, -1200, 1640)
        link(ripple_offset, ripple_direction)
        ripple_slope = binary(material, unreal.MaterialExpressionMultiply,
                              ripple_direction, ripple_strength, 1000, 1260)
        final_slope = binary(material, unreal.MaterialExpressionAdd,
                             weather_slope, ripple_slope, 1200, 360)

    normal_xy = expr(material, unreal.MaterialExpressionAppendVector, 600, 200)
    link(final_slope, normal_xy, "A")
    link(expr(material, unreal.MaterialExpressionConstant, 400, 360, r=1.0), normal_xy, "B")
    normalized = expr(material, unreal.MaterialExpressionNormalize, 800, 200)
    link(normal_xy, normalized)
    MEL.connect_material_property(normalized, "", unreal.MaterialProperty.MP_NORMAL)

    calm_color = (0.012, 0.085, 0.11) if IS_GRAZING_READABLE_CANDIDATE else (0.025, 0.17, 0.22)
    calm_pool_color = vector(material, "CalmPoolColor", calm_color, 100, -250)
    storm_pool_color = vector(material, "StormPoolColor", (0.006, 0.045, 0.08), 100, -100)
    pool_color = expr(material, unreal.MaterialExpressionLinearInterpolate, 360, -260)
    link(calm_pool_color, pool_color, "A")
    link(storm_pool_color, pool_color, "B")
    link(agitation, pool_color, "Alpha")
    edge_tint = (0.045, 0.16, 0.19) if IS_GRAZING_READABLE_CANDIDATE else (0.14, 0.43, 0.47)
    reflection_tint = vector(material, "EdgeReflectionTint", edge_tint, 300, -100)
    fresnel = expr(material, unreal.MaterialExpressionFresnel, 300, 50, exponent=4.0)
    color = expr(material, unreal.MaterialExpressionLinearInterpolate, 700, -180)
    link(pool_color, color, "A")
    link(reflection_tint, color, "B")
    link(fresnel, color, "Alpha")
    MEL.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)

    roughness = binary(material, unreal.MaterialExpressionAdd,
                       scalar(material, "CalmRoughness", 0.12, 700, -600),
                       binary(material, unreal.MaterialExpressionMultiply, agitation,
                              scalar(material, "WeatherRoughness", 0.62, 700, -450), 900, -450),
                       1100, -550)
    MEL.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(scalar(material, "Specular", 0.62, 1000, -300),
                                  "", unreal.MaterialProperty.MP_SPECULAR)

    errors = MEL.recompile_material(material)
    if errors:
        raise RuntimeError("Material compile errors: " + " | ".join(str(error) for error in errors))
    if not unreal.EditorAssetLibrary.save_asset(MATERIAL_PATH):
        raise RuntimeError("Could not save " + MATERIAL_PATH)
    unreal.log("[TideglassMaterial] Created additive visibly wavy pool material {}".format(MATERIAL_PATH))


try:
    build()
except Exception:
    unreal.log_error("[TideglassMaterial] " + traceback.format_exc())
