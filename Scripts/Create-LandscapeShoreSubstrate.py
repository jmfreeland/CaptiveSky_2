"""Substrate variant of the shoreline landscape material: a physically based water film instead of roughness tricks.

The project already runs Substrate (r.Substrate=True) but the landscape graph is legacy material attributes that Substrate
auto-converts, so wetness is faked by darkening and lowering roughness. Substrate (production-ready since 5.7, in 5.8.3 here)
can layer a real thin dielectric on top: this converts the finished legacy attributes with Substrate Convert Material Attributes
and puts a clear water slab on top with Substrate Vertical Layer, driven by a Substrate Coverage Weight. The film adds proper
Fresnel and sky reflection at grazing angles over the darkened ground, which is what makes wet sand and puddles read as wet.

Needs /Game/Materials/M_Island_Textured_Shore (Create-LandscapeShoreMaterial.py). Creates, never touching authored assets:
  /Game/Materials/M_Island_Textured_ShoreSubstrate     copy of the shore graph with the film added and Front Material connected
  /Game/Materials/MI_Island_Landscape_ShoreSubstrate   copy of MI_Island_Landscape_Shore reparented to it

Film coverage (parameters in group "Shore Film", tune on the instance):
  - puddles: the same tiled-noise pattern as the wet stage (FilmTileCm / FilmCoverage / FilmSharpness, flat ground only), scaled by
    environment Wetness from MPC_IslandEnvironment;
  - FilmSheen: a faint film everywhere on flat ground while it is wet;
  - waterline: sand within FilmShoreBandCm above sea level always carries a film (FilmShoreAmount).
The slab is a clear dielectric: DiffuseAlbedo 0, F0 0.02, F90 1, roughness FilmRoughness; its normal defaults to the vertex
normal, so the film stays flat over micro-relief.

This is an experiment: whether a Substrate Front Material is accepted by the landscape pipeline is checked by compiling, then by a
viewpoint capture (a checkerboard or a compile error means it is not). The legacy shore material is the safe choice.
Run like Create-LandscapeShoreMaterial.py (headless scratch editor); preview with
  Scripts/Capture-Viewpoints.ps1 -Hour 12 -Only TideglassGroundDetail -LandscapeParent /Game/Materials/M_Island_Textured_ShoreSubstrate
"""

import os
import traceback

import unreal

TAG = "[ShoreSubstrate] "
BASE_PARENT = "/Game/Materials/M_Island_Textured_Shore"
BASE_INSTANCE = "/Game/Materials/MI_Island_Landscape_Shore"
PARENT_OUT = "/Game/Materials/M_Island_Textured_ShoreSubstrate"
INSTANCE_OUT = "/Game/Materials/MI_Island_Landscape_ShoreSubstrate"
NOISE_TEXTURE = "/Game/Materals/Textures/T_LandscapeNoise"
COLLECTION_PATH = "/Game/Environment/MPC_IslandEnvironment"
WETNESS_PARAMETER = "Wetness"
GROUP = "Shore Film"
SEA_LEVEL_CM = 940.0
AUTHORED = "/Game/Materials/MI_Island_Landscape"
AUTHORED_BACKUP = "/Game/Materials/MI_Island_Landscape_PreShoreBackup"   # made by Create-LandscapeShoreMaterial.py
SHORE_PARENT = "/Game/Materials/M_Island_Textured_Shore"

MEL = unreal.MaterialEditingLibrary


def log(msg):
    unreal.log(TAG + msg)


def expr(material, cls, x, y, **props):
    node = MEL.create_material_expression(material, cls, x, y)
    if node is None:
        raise RuntimeError("Could not create " + str(cls))
    for key, value in props.items():
        node.set_editor_property(key, value)
    return node


def link(src, dst, dst_pin="", src_pin=""):
    if isinstance(src, tuple):
        src, src_pin = src
    if not MEL.connect_material_expressions(src, src_pin, dst, dst_pin):
        raise RuntimeError("Could not connect {}.{} -> {}.{}".format(src.get_class().get_name(), src_pin, dst.get_class().get_name(), dst_pin))


def link_any(src, dst, names):
    """Substrate nodes name their pins with spaces in places; try the likely spellings."""
    last = None
    for n in names:
        try:
            link(src, dst, n)
            return n
        except RuntimeError as err:
            last = err
    raise last


def binary(m, cls, a, b, x, y):
    node = expr(m, cls, x, y)
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


def maxn(m, a, b, x, y):
    return binary(m, unreal.MaterialExpressionMax, a, b, x, y)


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


def delete_if_exists(path):
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        if not unreal.EditorAssetLibrary.delete_asset(path):
            raise RuntimeError("Could not delete " + path)


def build():
    noise = unreal.load_asset(NOISE_TEXTURE)
    collection = unreal.load_asset(COLLECTION_PATH)
    if not (noise and collection and unreal.load_asset(BASE_PARENT) and unreal.load_asset(BASE_INSTANCE)):
        raise RuntimeError("Missing an input asset; run Create-LandscapeShoreMaterial.py first")
    # Authoring: LANDSCAPE_SHORE_AUTHOR=1 reparents the map's MI_Island_Landscape onto this Substrate graph; =0 puts it back on the
    # legacy shore graph (M_Island_Textured_Shore); unset leaves it as it is. The parent is deleted and rebuilt below, so detach first.
    mode = os.environ.get("LANDSCAPE_SHORE_AUTHOR", "")
    authored = unreal.load_asset(AUTHORED)
    authored_was_subs = bool(authored) and PARENT_OUT.rsplit("/", 1)[-1] in authored.get_editor_property("parent").get_path_name()
    if authored_was_subs:
        authored.set_editor_property("parent", unreal.load_asset(SHORE_PARENT))
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

    final_attrs = MEL.get_material_property_input_node(m, unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
    final_pin = MEL.get_material_property_input_node_output_name(m, unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
    if final_attrs is None:
        raise RuntimeError("Shore parent has nothing on its attributes output")
    log("legacy attributes come from {} (pin '{}')".format(final_attrs.get_class().get_name(), final_pin))

    X0, Y0 = 12000, -1200
    # ---------------------------------------------------------------- film coverage
    wet = saturate(m, expr(m, unreal.MaterialExpressionCollectionParameter, X0, Y0, collection=collection, parameter_name=WETNESS_PARAMETER), X0 + 300, Y0)
    world = expr(m, unreal.MaterialExpressionWorldPosition, X0, Y0 + 300)
    wxy = channel(m, world, X0 + 200, Y0 + 300, r=True, g=True)
    wz = channel(m, world, X0 + 200, Y0 + 420, b=True)
    uv = div(m, wxy, scalar(m, "FilmTileCm", 1200.0, X0 + 200, Y0 + 520), X0 + 450, Y0 + 400)
    ns = expr(m, unreal.MaterialExpressionTextureSample, X0 + 650, Y0 + 400, texture=noise, sampler_source=unreal.SamplerSourceMode.SSM_WRAP_WORLD_GROUP_SETTINGS)
    link(uv, ns, "UVs")
    nv = channel(m, ns, X0 + 850, Y0 + 400, r=True)
    thresh = mul(m, wet, scalar(m, "FilmCoverage", 0.36, X0 + 650, Y0 + 560), X0 + 1050, Y0 + 500)
    depth = mul(m, sub(m, thresh, nv, X0 + 1250, Y0 + 500), scalar(m, "FilmSharpness", 6.5, X0 + 1050, Y0 + 640), X0 + 1450, Y0 + 500)
    nz = channel(m, expr(m, unreal.MaterialExpressionVertexNormalWS, X0 + 1050, Y0 + 800), X0 + 1250, Y0 + 800, b=True)
    flat = saturate(m, mul(m, sub(m, nz, constant(m, 0.93, X0 + 1250, Y0 + 900), X0 + 1450, Y0 + 800), constant(m, 25.0, X0 + 1450, Y0 + 900), X0 + 1650, Y0 + 800), X0 + 1850, Y0 + 800)
    pool = mul(m, saturate(m, depth, X0 + 1650, Y0 + 500), flat, X0 + 2050, Y0 + 650)
    sheen = mul(m, mul(m, wet, scalar(m, "FilmSheen", 0.25, X0 + 1450, Y0 + 1050), X0 + 1650, Y0 + 1000), flat, X0 + 2050, Y0 + 1000)
    rain_film = saturate(m, add(m, pool, sheen, X0 + 2250, Y0 + 800), X0 + 2450, Y0 + 800)

    rel_h = sub(m, wz, scalar(m, "FilmSeaLevelCm", SEA_LEVEL_CM, X0 + 200, Y0 + 1200), X0 + 450, Y0 + 1250)
    band = scalar(m, "FilmShoreBandCm", 60.0, X0 + 450, Y0 + 1400)
    shore_ramp = saturate(m, div(m, sub(m, band, rel_h, X0 + 700, Y0 + 1300), band, X0 + 900, Y0 + 1300), X0 + 1100, Y0 + 1300)
    above_floor = saturate(m, div(m, add(m, rel_h, constant(m, 40.0, X0 + 700, Y0 + 1500), X0 + 900, Y0 + 1500), constant(m, 40.0, X0 + 900, Y0 + 1600), X0 + 1100, Y0 + 1500), X0 + 1300, Y0 + 1500)
    shore_film = mul(m, mul(m, shore_ramp, above_floor, X0 + 1500, Y0 + 1400), mul(m, flat, scalar(m, "FilmShoreAmount", 0.7, X0 + 1500, Y0 + 1550), X0 + 1700, Y0 + 1500), X0 + 1900, Y0 + 1450)
    coverage = saturate(m, maxn(m, rain_film, shore_film, X0 + 2650, Y0 + 1100), X0 + 2850, Y0 + 1100)

    # ---------------------------------------------------------------- Substrate stack
    conv = expr(m, unreal.MaterialExpressionSubstrateConvertMaterialAttributes, X0 + 3000, Y0)
    link(final_attrs, conv, "", final_pin)
    film = expr(m, unreal.MaterialExpressionSubstrateSlabBSDF, X0 + 3000, Y0 + 700)
    used = {
        "DiffuseAlbedo": link_any(expr(m, unreal.MaterialExpressionConstant3Vector, X0 + 2850, Y0 + 700, constant=unreal.LinearColor(0, 0, 0, 1)), film, ["Diffuse Albedo", "DiffuseAlbedo"]),
        "F0": link_any(expr(m, unreal.MaterialExpressionConstant3Vector, X0 + 2850, Y0 + 800, constant=unreal.LinearColor(0.02, 0.02, 0.02, 1)), film, ["F0"]),
        "F90": link_any(expr(m, unreal.MaterialExpressionConstant3Vector, X0 + 2850, Y0 + 900, constant=unreal.LinearColor(1, 1, 1, 1)), film, ["F90"]),
        "Roughness": link_any(scalar(m, "FilmRoughness", 0.03, X0 + 2850, Y0 + 1000), film, ["Roughness"]),
    }
    weighted = expr(m, unreal.MaterialExpressionSubstrateWeight, X0 + 3300, Y0 + 700)
    link_any(film, weighted, ["A"])
    link_any(coverage, weighted, ["Weight"])
    # Parameter blending merges the film and the ground into ONE BSDF by mixing their inputs instead of evaluating both layers.
    # Without it this variant cost +70% pixel-shader instructions over the wet graph; it is the Substrate cost lever for a surface that covers the screen.
    layer = expr(m, unreal.MaterialExpressionSubstrateVerticalLayering, X0 + 3600, Y0 + 300, use_parameter_blending=True)
    log("vertical layer parameter blending: {}".format(layer.get_editor_property("use_parameter_blending")))
    link_any(weighted, layer, ["Top"])
    link_any(conv, layer, ["Bottom", "Base"])   # the header calls it Base; the editor shows Bottom
    log("pins used: {}".format(used))

    ok = MEL.connect_material_property(layer, "", unreal.MaterialProperty.MP_FRONT_MATERIAL)
    if not ok:
        raise RuntimeError("Could not connect the Substrate stack to Front Material")
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
    log("built {} -> {}".format(INSTANCE_OUT, instance.get_editor_property("parent").get_path_name()))
    if mode == "1" or (authored_was_subs and mode != "0"):
        authored.set_editor_property("parent", m)
        MEL.update_material_instance(authored)
        if not unreal.EditorAssetLibrary.save_asset(AUTHORED):
            raise RuntimeError("Could not save " + AUTHORED)
    log("{} now uses {}".format(AUTHORED, authored.get_editor_property("parent").get_path_name()))


try:
    build()
    log("DONE")
except Exception:
    unreal.log_error(TAG + traceback.format_exc())
