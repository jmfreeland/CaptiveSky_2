"""Creates M_IslandRainbow, the additive unlit material for the rainbow plane.

The plane (see IslandRainbow.cpp) faces the viewer with the anti-solar point at its centre, so the
bows are concentric rings in UV space: a bright primary bow (red outside) and a faint reversed
secondary bow further out. Scalar parameters Intensity and HorizonZ are driven by the runtime.
The asset is local-only (Content/ is gitignored); rerun this script to rebuild it. It refuses to
overwrite an existing asset: delete /Game/Materials/M_IslandRainbow first to rebuild.

Run headless:
  UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="<this file>"
or in an open editor: py "<this file>"
"""

import math
import os
import traceback
import unreal

MATERIAL_NAME = os.environ.get("CAPTIVESKY_RAINBOW_MATERIAL_NAME", "M_IslandRainbow")
MATERIAL_PATH = "/Game/Materials/" + MATERIAL_NAME
MEL = unreal.MaterialEditingLibrary

# The plane is 2.7 * Distance wide, so UV radius r maps to a viewing angle via
# tan(angle) = (r * 2.7) in units of Distance.
PLANE_SPAN = 2.7


def uv_radius_for_degrees(degrees):
    return math.tan(math.radians(degrees)) / PLANE_SPAN


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
                parameter_name=name, default_value=value, group="Rainbow")


def unary(material, cls, a, x, y):
    node = expr(material, cls, x, y)
    link(a, node)
    return node


def sine_of(material, value, x, y):
    # MaterialExpressionSine computes sin(value * 2*pi / period); a period of 2*pi gives plain sin(value).
    node = expr(material, unreal.MaterialExpressionSine, x, y, period=2.0 * math.pi)
    link(value, node)
    return node


def hue_channel(material, hue, offset_scale_node, kind, x, y):
    """HSV hue (0..1) to one RGB channel: R = sat(|6h-3|-1), G = sat(2-|6h-2|), B = sat(2-|6h-4|)."""
    six = binary(material, unreal.MaterialExpressionMultiply, hue, const(material, 6.0, x, y + 60), x + 120, y)
    if kind == "R":
        centred = binary(material, unreal.MaterialExpressionSubtract, six, const(material, 3.0, x, y + 120), x + 240, y)
        shaped = binary(material, unreal.MaterialExpressionSubtract,
                        unary(material, unreal.MaterialExpressionAbs, centred, x + 360, y),
                        const(material, 1.0, x + 360, y + 60), x + 480, y)
    else:
        centre = 2.0 if kind == "G" else 4.0
        centred = binary(material, unreal.MaterialExpressionSubtract, six, const(material, centre, x, y + 120), x + 240, y)
        shaped = binary(material, unreal.MaterialExpressionSubtract,
                        const(material, 2.0, x + 360, y + 60),
                        unary(material, unreal.MaterialExpressionAbs, centred, x + 360, y), x + 480, y)
    return unary(material, unreal.MaterialExpressionSaturate, shaped, x + 600, y)


def band(material, radius, inner, outer, reversed_colours, peak, x, y):
    """One bow: a soft sine envelope across [inner, outer] with a red-to-violet ramp across it."""
    t = binary(material, unreal.MaterialExpressionDivide,
               binary(material, unreal.MaterialExpressionSubtract, radius, const(material, inner, x, y + 40), x + 100, y),
               const(material, outer - inner, x, y + 100), x + 220, y)
    t_clamped = expr(material, unreal.MaterialExpressionClamp, x + 340, y, min_default=-0.05, max_default=1.05)
    link(t, t_clamped)
    envelope_phase = binary(material, unreal.MaterialExpressionMultiply, t_clamped, const(material, math.pi, x + 340, y + 60), x + 460, y)
    envelope = expr(material, unreal.MaterialExpressionPower, x + 820, y)
    # Sharpen the envelope a little so the bands read as arcs rather than a glow.
    link(unary(material, unreal.MaterialExpressionSaturate,
               sine_of(material, envelope_phase, x + 580, y), x + 700, y), envelope, "Base")
    link(const(material, 0.8, x + 700, y + 60), envelope, "Exp")
    # t = 1 is the outside of the primary bow, which is red (hue 0); violet sits at hue 0.75.
    along = t_clamped if reversed_colours else binary(material, unreal.MaterialExpressionSubtract,
                                                      const(material, 1.0, x + 340, y + 120), t_clamped, x + 460, y + 120)
    hue = binary(material, unreal.MaterialExpressionMultiply, along, const(material, 0.75, x + 460, y + 180), x + 580, y + 140)
    rgb = expr(material, unreal.MaterialExpressionAppendVector, x + 1500, y)
    rg = expr(material, unreal.MaterialExpressionAppendVector, x + 1380, y)
    link(hue_channel(material, hue, None, "R", x + 700, y + 160), rg, "A")
    link(hue_channel(material, hue, None, "G", x + 700, y + 360), rg, "B")
    link(rg, rgb, "A")
    link(hue_channel(material, hue, None, "B", x + 700, y + 560), rgb, "B")
    strength = binary(material, unreal.MaterialExpressionMultiply, envelope, const(material, peak, x + 940, y + 60), x + 1100, y)
    return binary(material, unreal.MaterialExpressionMultiply, rgb, strength, x + 1620, y)


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
    for name, value in (("apply_fogging", False), ("disable_depth_test", False)):
        try:
            material.set_editor_property(name, value)
        except Exception:
            unreal.log_warning("[RainbowMaterial] could not set " + name)

    uv = expr(material, unreal.MaterialExpressionTextureCoordinate, -1800, 0)
    centre = expr(material, unreal.MaterialExpressionConstant2Vector, -1800, 120, r=0.5, g=0.5)
    radius = binary(material, unreal.MaterialExpressionDistance, uv, centre, -1600, 0)

    primary = band(material, radius, uv_radius_for_degrees(40.0), uv_radius_for_degrees(43.5), False, 1.0, -1500, 300)
    secondary = band(material, radius, uv_radius_for_degrees(50.0), uv_radius_for_degrees(54.0), True, 0.22, -1500, 1300)
    bows = binary(material, unreal.MaterialExpressionAdd, primary, secondary, 400, 600)

    # Fade out toward and below the viewer's horizon (HorizonZ is the viewer's height, set at runtime).
    world_z = unary(material, unreal.MaterialExpressionComponentMask,
                    expr(material, unreal.MaterialExpressionWorldPosition, -600, -300), -450, -300)
    world_z.set_editor_property("r", False)
    world_z.set_editor_property("g", False)
    world_z.set_editor_property("b", True)
    height = binary(material, unreal.MaterialExpressionSubtract, world_z,
                    scalar(material, "HorizonZ", 0.0, -450, -200), -250, -300)
    above = unary(material, unreal.MaterialExpressionSaturate,
                  binary(material, unreal.MaterialExpressionDivide, height, const(material, 1500.0, -250, -240), -50, -300), 100, -300)
    lit = binary(material, unreal.MaterialExpressionMultiply, bows, above, 600, 600)
    intensity = scalar(material, "Intensity", 0.0, 400, 760)
    final = binary(material, unreal.MaterialExpressionMultiply, lit,
                   binary(material, unreal.MaterialExpressionMultiply, intensity, scalar(material, "Brightness", 1.4, 400, 840), 600, 760), 800, 600)
    MEL.connect_material_property(final, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    errors = MEL.recompile_material(material)
    if errors:
        raise RuntimeError("Material compile errors: " + " | ".join(str(error) for error in errors))
    if not unreal.EditorAssetLibrary.save_asset(MATERIAL_PATH):
        raise RuntimeError("Could not save " + MATERIAL_PATH)
    unreal.log("[RainbowMaterial] Created " + MATERIAL_PATH)


try:
    build()
except Exception:
    unreal.log_error("[RainbowMaterial] " + traceback.format_exc())
