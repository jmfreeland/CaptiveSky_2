"""Builds a wet-ground variant of the Island landscape material, leaving the authored assets untouched.

Creates /Game/Materials/M_Island_Textured_Wet (a copy of M_Island_Textured_Auto) and
/Game/Materials/MI_Island_Landscape_Wet (a copy of MI_Island_Landscape reparented to it). The copy gets an extra
stage on its final material attributes that reads the `Wetness` entry of MPC_IslandEnvironment, which
UIslandEnvironmentSubsystem drives from rain, so wet weather changes the ground with no per-component code:

  - the whole ground darkens and loses roughness as wetness rises;
  - on flat ground a tiled noise mask pools water: darker, near-mirror roughness, flattened normals.

The stock `Ground Wetness` only moves specular and the stock `Add Puddles` branch showed no visible response
(docs/findings/2026-09-28-landscape-material-audit.md), hence this separate stage. Rebuilds its own two assets every run.

Preview: Scripts/Capture-Viewpoints.ps1 -Hour 12 -Only TideglassGroundDetail -CompareLandscapeWetness
         -LandscapeParent /Game/Materials/M_Island_Textured_Wet
In the editor and PIE, UIslandEnvironmentSubsystem swaps this graph in at runtime; standalone and packaged builds do not.
"""

import traceback
import unreal

PARENT_SOURCE = "/Game/Materials/M_Island_Textured_Auto"
INSTANCE_SOURCE = "/Game/Materials/MI_Island_Landscape"
PARENT_COPY = "/Game/Materials/M_Island_Textured_Wet"
INSTANCE_COPY = "/Game/Materials/MI_Island_Landscape_Wet"
NOISE_TEXTURE = "/Game/Materals/Textures/T_LandscapeNoise"
WETNESS_PARAMETER = "Wetness"
COLLECTION_PATH = "/Game/Environment/MPC_IslandEnvironment"
GROUP = "Wet Response"

MEL = unreal.MaterialEditingLibrary


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


def mul(material, a, b, x, y):
    return binary(material, unreal.MaterialExpressionMultiply, a, b, x, y)


def sub(material, a, b, x, y):
    return binary(material, unreal.MaterialExpressionSubtract, a, b, x, y)


def lerp(material, a, b, alpha, x, y):
    node = expr(material, unreal.MaterialExpressionLinearInterpolate, x, y)
    link(a, node, "A")
    link(b, node, "B")
    link(alpha, node, "Alpha")
    return node


def saturate(material, source, x, y):
    node = expr(material, unreal.MaterialExpressionSaturate, x, y)
    link(source, node)
    return node


def constant(material, value, x, y):
    return expr(material, unreal.MaterialExpressionConstant, x, y, r=value)


def scalar(material, name, value, x, y):
    return expr(material, unreal.MaterialExpressionScalarParameter, x, y, parameter_name=name, default_value=value, group=GROUP)


def channel(material, source, x, y, r=False, g=False, b=False):
    mask = expr(material, unreal.MaterialExpressionComponentMask, x, y, r=r, g=g, b=b)
    link(source, mask)
    return mask


def delete_if_exists(path):
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        if not unreal.EditorAssetLibrary.delete_asset(path):
            raise RuntimeError("Could not delete " + path)


def build():
    noise = unreal.load_asset(NOISE_TEXTURE)
    if not noise:
        raise RuntimeError("Missing noise texture " + NOISE_TEXTURE)
    collection = unreal.load_asset(COLLECTION_PATH)
    if not collection:
        raise RuntimeError("Missing collection " + COLLECTION_PATH)
    delete_if_exists(INSTANCE_COPY)
    delete_if_exists(PARENT_COPY)
    material = unreal.EditorAssetLibrary.duplicate_asset(PARENT_SOURCE, PARENT_COPY)
    if not material:
        raise RuntimeError("Could not duplicate " + PARENT_SOURCE)
    if not material.get_editor_property("use_material_attributes"):
        raise RuntimeError("Parent does not output material attributes; this script expects it to")

    source_node = MEL.get_material_property_input_node(material, unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
    if source_node is None:
        raise RuntimeError("Nothing is connected to the material attributes output")
    source_pin = MEL.get_material_property_input_node_output_name(material, unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
    unreal.log("[LandscapeWet] Final attributes come from {} (pin '{}')".format(source_node.get_class().get_name(), source_pin))

    broken = expr(material, unreal.MaterialExpressionBreakMaterialAttributes, 4000, 0)
    link(source_node, broken, "", source_pin)

    # Environment wetness 0..1, driven by UIslandEnvironmentSubsystem through the shared collection. Landscape
    # components render from map-baked instances, so a per-component MID never reaches the screen; a collection does.
    wet = saturate(material, expr(material, unreal.MaterialExpressionCollectionParameter, 3000, 600,
                                  collection=collection, parameter_name=WETNESS_PARAMETER), 3600, 600)

    # Pools: tiled noise below a wetness-dependent threshold, on flat ground only.
    world = expr(material, unreal.MaterialExpressionWorldPosition, 3000, 900)
    world_xy = channel(material, world, 3200, 900, r=True, g=True)
    uv = binary(material, unreal.MaterialExpressionDivide, world_xy, scalar(material, "PuddleTileCm", 900.0, 3200, 1000), 3400, 900)
    noise_sample = expr(material, unreal.MaterialExpressionTextureSample, 3600, 900, texture=noise,
                        sampler_source=unreal.SamplerSourceMode.SSM_WRAP_WORLD_GROUP_SETTINGS)
    link(uv, noise_sample, "UVs")
    noise_value = channel(material, noise_sample, 3800, 900, r=True)
    threshold = mul(material, wet, scalar(material, "PuddleCoverage", 0.4, 3600, 1050), 3800, 1000)
    depth = mul(material, sub(material, threshold, noise_value, 4000, 1000), scalar(material, "PuddleSharpness", 8.0, 3800, 1100), 4200, 1000)
    normal_z = channel(material, expr(material, unreal.MaterialExpressionVertexNormalWS, 3800, 1250), 4000, 1250, b=True)
    flat = saturate(material, mul(material, sub(material, normal_z, constant(material, 0.93, 4000, 1350), 4200, 1250),
                                  constant(material, 25.0, 4200, 1350), 4400, 1250), 4600, 1250)
    pool = mul(material, saturate(material, depth, 4400, 1000), flat, 4800, 1100)

    # Ground-wide darkening and roughness loss, then the pools on top.
    dark = lerp(material, constant(material, 1.0, 4200, 200), scalar(material, "WetDarken", 0.6, 4200, 260), wet, 4400, 200)
    pool_dark = lerp(material, constant(material, 1.0, 4800, 300), scalar(material, "PuddleDarken", 0.55, 4800, 360), pool, 5000, 300)

    make = expr(material, unreal.MaterialExpressionMakeMaterialAttributes, 6400, 0)

    base_color = mul(material, mul(material, (broken, "BaseColor"), dark, 5200, 100), pool_dark, 5600, 100)
    link(base_color, make, "BaseColor")

    roughness_in = (broken, "Roughness")
    roughness_wet = lerp(material, roughness_in, mul(material, roughness_in, scalar(material, "WetRoughnessScale", 0.45, 4800, 520), 5000, 500), wet, 5200, 450)
    roughness = lerp(material, roughness_wet, scalar(material, "PuddleRoughness", 0.04, 5200, 600), pool, 5600, 450)
    link(roughness, make, "Roughness")

    flat_normal = expr(material, unreal.MaterialExpressionConstant3Vector, 5200, 800, constant=unreal.LinearColor(0.0, 0.0, 1.0, 1.0))
    link(lerp(material, (broken, "Normal"), flat_normal, pool, 5600, 800), make, "Normal")

    specular = lerp(material, (broken, "Specular"), constant(material, 0.5, 5200, 1550), pool, 5600, 1500)
    link(specular, make, "Specular")

    for passthrough in ("Metallic", "AmbientOcclusion"):
        link(broken, make, passthrough, passthrough)

    MEL.connect_material_property(make, "", unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES)
    MEL.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_asset(PARENT_COPY):
        raise RuntimeError("Could not save " + PARENT_COPY)

    instance = unreal.EditorAssetLibrary.duplicate_asset(INSTANCE_SOURCE, INSTANCE_COPY)
    if not instance:
        raise RuntimeError("Could not duplicate " + INSTANCE_SOURCE)
    instance.set_editor_property("parent", material)
    MEL.update_material_instance(instance)
    if not unreal.EditorAssetLibrary.save_asset(INSTANCE_COPY):
        raise RuntimeError("Could not save " + INSTANCE_COPY)

    authored = unreal.load_asset(INSTANCE_SOURCE)
    authored_parent = authored.get_editor_property("parent").get_path_name()
    copy_parent = instance.get_editor_property("parent").get_path_name()
    unreal.log("[LandscapeWet] Built {} -> parent {}; authored instance still uses {}".format(INSTANCE_COPY, copy_parent, authored_parent))
    if PARENT_SOURCE.rsplit("/", 1)[-1] not in authored_parent:
        raise RuntimeError("Authored instance parent changed unexpectedly: " + authored_parent)


try:
    build()
except Exception:
    unreal.log_error("[LandscapeWet] " + traceback.format_exc())
