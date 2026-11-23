"""Gives the inn's exterior finishes and the stone props the same rain response as the wet landscape.

Each material gets a stage on its BaseColor and Roughness that reads `Wetness` (and `Indoors`) from
MPC_IslandEnvironment, so it needs no per-actor code and works in standalone builds:

  - colour darkens as wetness rises;
  - roughness drops, and drops further on upward-facing surfaces (roof, steps) where water sheets;
  - the response fades out while the viewer is indoors, so interior plaster and timber stay dry-looking.

Targets (all local-only Content; this folder's Content is gitignored):
  /Game/Materials/M_Island_WetColor     new master with Color and Roughness parameters (like BasicShapeMaterial),
                                        rebuilt every run; MI_Inn_Plaster/Timber/Shingle are reparented onto it
                                        (revert: set their parent back to /Engine/BasicShapes/BasicShapeMaterial)
  /Game/StarterContent/Props/Materials/M_Rock   edited in place once (inn base, steps, chimney); the original is kept as
                                        M_Rock_AuthoredBackup beside it, and the script skips M_Rock when that exists

At wetness 0 every material renders as before.

Run in the full editor: UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript=<this file> -unattended -NoZen
"""

import traceback
import unreal

COLLECTION_PATH = "/Game/Environment/MPC_IslandEnvironment"
MASTER = "/Game/Materials/M_Island_WetColor"
ENGINE_BASIC = "/Engine/BasicShapes/BasicShapeMaterial"
INSTANCES = ["/Game/Inn/Materials/MI_Inn_Plaster", "/Game/Inn/Materials/MI_Inn_Timber", "/Game/Inn/Materials/MI_Inn_Shingle"]
ROCK = "/Game/StarterContent/Props/Materials/M_Rock"
ROCK_BACKUP = "/Game/StarterContent/Props/Materials/M_Rock_AuthoredBackup"
GROUP = "Wet Response"

MEL = unreal.MaterialEditingLibrary
MP = unreal.MaterialProperty


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


def binary(material, cls, a, b, x, y):
    node = expr(material, cls, x, y)
    link(a, node, "A")
    link(b, node, "B")
    return node


def mul(material, a, b, x, y):
    return binary(material, unreal.MaterialExpressionMultiply, a, b, x, y)


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


def input_of(material, prop):
    node = MEL.get_material_property_input_node(material, prop)
    if node is None:
        raise RuntimeError("{} has nothing connected to {}".format(material.get_name(), prop))
    return node, MEL.get_material_property_input_node_output_name(material, prop)


def add_wet_stage(material, collection, x0=2000, darken=0.6, roughness_scale=0.45, up_gloss=0.55):
    """Wraps the material's current BaseColor and Roughness with the wet response."""
    color = input_of(material, MP.MP_BASE_COLOR)
    roughness = input_of(material, MP.MP_ROUGHNESS)

    environment_wet = saturate(material, expr(material, unreal.MaterialExpressionCollectionParameter, x0, 600, collection=collection, parameter_name="Wetness"), x0 + 200, 600)
    indoors = saturate(material, expr(material, unreal.MaterialExpressionCollectionParameter, x0, 700, collection=collection, parameter_name="Indoors"), x0 + 200, 700)
    outdoors = expr(material, unreal.MaterialExpressionOneMinus, x0 + 400, 700)
    link(indoors, outdoors)
    wet = mul(material, environment_wet, outdoors, x0 + 600, 650)

    dark = lerp(material, constant(material, 1.0, x0 + 600, 100), scalar(material, "WetDarken", darken, x0 + 600, 160), wet, x0 + 800, 100)
    wet_color = mul(material, color, dark, x0 + 1000, 100)

    up_z = expr(material, unreal.MaterialExpressionComponentMask, x0 + 600, 900, b=True)
    link(expr(material, unreal.MaterialExpressionVertexNormalWS, x0 + 400, 900), up_z)
    up_amount = saturate(material, up_z, x0 + 800, 900)
    sheen = mul(material, wet, mul(material, up_amount, scalar(material, "UpGloss", up_gloss, x0 + 800, 960), x0 + 1000, 900), x0 + 1200, 900)
    scaled = mul(material, roughness, lerp(material, constant(material, 1.0, x0 + 600, 300), scalar(material, "WetRoughnessScale", roughness_scale, x0 + 600, 360), wet, x0 + 800, 300), x0 + 1000, 300)
    wet_roughness = mul(material, scaled, lerp(material, constant(material, 1.0, x0 + 1000, 500), constant(material, 0.5, x0 + 1000, 560), sheen, x0 + 1200, 500), x0 + 1400, 300)

    MEL.connect_material_property(wet_color, "", MP.MP_BASE_COLOR)
    MEL.connect_material_property(wet_roughness, "", MP.MP_ROUGHNESS)


def save(path):
    if not unreal.EditorAssetLibrary.save_asset(path):
        raise RuntimeError("Could not save " + path)


def build_master(collection):
    engine = unreal.load_asset(ENGINE_BASIC)
    roughness_default = 0.5
    rough_node, _ = input_of(engine, MP.MP_ROUGHNESS)
    if isinstance(rough_node, unreal.MaterialExpressionScalarParameter):
        roughness_default = rough_node.get_editor_property("default_value")

    # Detach instances: the master is deleted and rebuilt, and nothing may point at it meanwhile.
    for path in INSTANCES:
        instance = unreal.load_asset(path)
        if not instance:
            raise RuntimeError("Missing " + path)
        instance.set_editor_property("parent", engine)
        MEL.update_material_instance(instance)
        save(path)
    if unreal.EditorAssetLibrary.does_asset_exist(MASTER) and not unreal.EditorAssetLibrary.delete_asset(MASTER):
        raise RuntimeError("Could not delete " + MASTER)

    folder, name = MASTER.rsplit("/", 1)
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    if not material:
        raise RuntimeError("Could not create " + MASTER)
    color = expr(material, unreal.MaterialExpressionVectorParameter, 0, 0, parameter_name="Color", default_value=unreal.LinearColor(0.5, 0.5, 0.5, 1.0))
    rough = expr(material, unreal.MaterialExpressionScalarParameter, 0, 300, parameter_name="Roughness", default_value=roughness_default)
    MEL.connect_material_property(color, "RGB", MP.MP_BASE_COLOR)
    MEL.connect_material_property(rough, "", MP.MP_ROUGHNESS)
    add_wet_stage(material, collection)
    MEL.recompile_material(material)
    save(MASTER)

    for path in INSTANCES:
        instance = unreal.load_asset(path)
        instance.set_editor_property("parent", material)
        MEL.update_material_instance(instance)
        save(path)
        unreal.log("[WetSurfaces] {} -> {}".format(path, instance.get_editor_property("parent").get_path_name()))


def author_rock(collection):
    if unreal.EditorAssetLibrary.does_asset_exist(ROCK_BACKUP):
        unreal.log("[WetSurfaces] {} already has a backup; leaving it as authored".format(ROCK))
        return
    if not unreal.EditorAssetLibrary.duplicate_asset(ROCK, ROCK_BACKUP):
        raise RuntimeError("Could not back up " + ROCK)
    save(ROCK_BACKUP)
    rock = unreal.load_asset(ROCK)
    add_wet_stage(rock, collection, x0=1600, darken=0.55, roughness_scale=0.5, up_gloss=0.5)
    MEL.recompile_material(rock)
    save(ROCK)
    unreal.log("[WetSurfaces] Added the wet stage to " + ROCK)


def build():
    collection = unreal.load_asset(COLLECTION_PATH)
    if not collection:
        raise RuntimeError("Missing collection " + COLLECTION_PATH)
    build_master(collection)
    author_rock(collection)


try:
    build()
except Exception:
    unreal.log_error("[WetSurfaces] " + traceback.format_exc())
