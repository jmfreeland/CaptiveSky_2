"""Creates M_IslandDew, the additive unlit material for the dew glints (see IslandDew.cpp).

Each glint is a tiny sphere instance. Its brightness is a sharp flash that repeats at a per-instance
rate and phase (from PerInstanceRandom), so the grass twinkles. Scalar parameters Intensity (driven by
the runtime) and Brightness (tuning) scale the result. The asset is local-only (Content/ is
gitignored); rerun this script to rebuild it. It refuses to overwrite an existing asset: delete
/Game/Materials/M_IslandDew first to rebuild.

Run headless:
  UnrealEditor-Cmd.exe <uproject> -ExecCmds="py <this file>" -unattended -NoSplash -NoZen -RenderOffscreen
or in an open editor: py "<this file>"
"""

import math
import os
import traceback
import unreal

MATERIAL_NAME = os.environ.get("CAPTIVESKY_DEW_MATERIAL_NAME", "M_IslandDew")
MATERIAL_PATH = "/Game/Materials/" + MATERIAL_NAME
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


def binary(material, cls, a, b, x, y):
    node = expr(material, cls, x, y)
    link(a, node, "A")
    link(b, node, "B")
    return node


def const(material, value, x, y):
    return expr(material, unreal.MaterialExpressionConstant, x, y, r=value)


def scalar(material, name, value, x, y):
    return expr(material, unreal.MaterialExpressionScalarParameter, x, y,
                parameter_name=name, default_value=value, group="Dew")


def build():
    if unreal.EditorAssetLibrary.does_asset_exist(MATERIAL_PATH):
        raise RuntimeError("Refusing to overwrite existing asset " + MATERIAL_PATH)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    material = tools.create_asset(MATERIAL_NAME, "/Game/Materials", unreal.Material, unreal.MaterialFactoryNew())
    if not material:
        raise RuntimeError("Could not create " + MATERIAL_PATH)

    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("two_sided", True)
    # The glints are instanced meshes; without this flag the engine falls back to the default material.
    material.set_editor_property("used_with_instanced_static_meshes", True)

    rand = expr(material, unreal.MaterialExpressionPerInstanceRandom, -1400, 0)
    time = expr(material, unreal.MaterialExpressionTime, -1400, 160)

    # Flash rate 1.2 .. 3.6 rad/s-ish, phase from the random value; period 2*pi makes Sine a plain sin.
    rate = binary(material, unreal.MaterialExpressionAdd,
                  binary(material, unreal.MaterialExpressionMultiply, rand, const(material, 2.4, -1400, 80), -1200, 0),
                  const(material, 1.2, -1200, 80), -1000, 0)
    phase = binary(material, unreal.MaterialExpressionMultiply, rand, const(material, 2.0 * math.pi * 7.0, -1200, 220), -1000, 220)
    angle = binary(material, unreal.MaterialExpressionAdd,
                   binary(material, unreal.MaterialExpressionMultiply, time, rate, -800, 100), phase, -600, 100)
    wave = expr(material, unreal.MaterialExpressionSine, -400, 100, period=2.0 * math.pi)
    link(angle, wave)
    positive = expr(material, unreal.MaterialExpressionSaturate, -250, 100)
    link(wave, positive)
    flash = expr(material, unreal.MaterialExpressionPower, -100, 100)
    link(positive, flash, "Base")
    link(const(material, 10.0, -250, 200), flash, "Exp")

    # A faint steady glow under the flashes so the beads read even between twinkles.
    level = binary(material, unreal.MaterialExpressionAdd, flash, const(material, 0.08, -100, 220), 60, 120)

    # Mostly warm white, a little shifted per instance so some beads catch pink and some blue-green.
    tint = expr(material, unreal.MaterialExpressionConstant3Vector, -100, -120, constant=unreal.LinearColor(1.0, 0.93, 0.78, 1.0))
    hue_shift = binary(material, unreal.MaterialExpressionAdd,
                       const(material, 0.85, -100, -260),
                       binary(material, unreal.MaterialExpressionMultiply, rand, const(material, 0.3, -300, -260), -100, -200), 100, -240)
    color = binary(material, unreal.MaterialExpressionMultiply, tint, hue_shift, 300, -140)

    lit = binary(material, unreal.MaterialExpressionMultiply, color, level, 500, 0)
    intensity = scalar(material, "Intensity", 0.0, 300, 260)
    brightness = scalar(material, "Brightness", 9.0, 300, 340)
    final = binary(material, unreal.MaterialExpressionMultiply, lit,
                   binary(material, unreal.MaterialExpressionMultiply, intensity, brightness, 520, 300), 720, 0)
    MEL.connect_material_property(final, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    errors = MEL.recompile_material(material)
    if errors:
        raise RuntimeError("Material compile errors: " + " | ".join(str(error) for error in errors))
    if not unreal.EditorAssetLibrary.save_asset(MATERIAL_PATH):
        raise RuntimeError("Could not save " + MATERIAL_PATH)
    unreal.log("[DewMaterial] Created " + MATERIAL_PATH)


try:
    build()
except Exception:
    unreal.log_error("[DewMaterial] " + traceback.format_exc())
