"""Create a thresholded, high-contrast view of the Island puddle noise."""

import traceback
import unreal


MATERIAL_PATH = "/Game/Materials/M_Island_PuddleNoiseContrastDebug"
SOURCE_PATH = "/Game/Materials/M_Island_Textured_Auto"
TEXTURE_PATH = "/Game/Materals/Textures/T_LandscapeNoise"
NOISE_TILE_CM = 200.0
THRESHOLD = 0.80
CONTRAST = 10.0

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
        raise RuntimeError("Could not connect {} -> {}.{}".format(
            src.get_class().get_name(), dst.get_class().get_name(), dst_pin))


def scalar(material, name, value, x, y):
    return expr(material, unreal.MaterialExpressionScalarParameter, x, y,
                parameter_name=name, default_value=value,
                group="Puddle contrast diagnostic")


def binary(material, cls, a, b, x, y):
    node = expr(material, cls, x, y)
    link(a, node, "A")
    link(b, node, "B")
    return node


def build():
    if unreal.EditorAssetLibrary.does_asset_exist(MATERIAL_PATH):
        raise RuntimeError("Refusing to overwrite existing diagnostic: " + MATERIAL_PATH)
    source = unreal.load_asset(SOURCE_PATH)
    texture = unreal.load_asset(TEXTURE_PATH)
    if not source or not texture:
        raise RuntimeError("Could not load the authored landscape parent or puddle-noise texture")

    # Retain the valid landscape physical-material setup. This copy is never assigned to
    # the map; only its visible attributes are replaced for a diagnostic capture.
    material = unreal.EditorAssetLibrary.duplicate_asset(SOURCE_PATH, MATERIAL_PATH)
    if not material or material.get_class().get_name() != "Material":
        raise RuntimeError("Could not duplicate the working landscape parent")
    material.set_editor_property("use_material_attributes", True)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)

    coords = expr(material, unreal.MaterialExpressionLandscapeLayerCoords, -1000, 0)
    tile = scalar(material, "Noise Tile Cm", NOISE_TILE_CM, -1000, 200)
    uv = binary(material, unreal.MaterialExpressionDivide, coords, tile, -760, 0)
    sample = expr(material, unreal.MaterialExpressionTextureSample, -520, 0, texture=texture)
    link(uv, sample, "UVs")
    red = expr(material, unreal.MaterialExpressionComponentMask, -300, 0,
               r=True, g=False, b=False, a=False)
    link(sample, red)

    centered = binary(material, unreal.MaterialExpressionSubtract, red,
                      scalar(material, "Noise Threshold", THRESHOLD, -300, 200), -60, 0)
    amplified = binary(material, unreal.MaterialExpressionMultiply, centered,
                       scalar(material, "Noise Contrast", CONTRAST, -60, 200), 180, 0)
    biased = binary(material, unreal.MaterialExpressionAdd, amplified,
                    expr(material, unreal.MaterialExpressionConstant, 180, 200, r=0.5), 420, 0)
    gray = expr(material, unreal.MaterialExpressionSaturate, 660, 0)
    link(biased, gray)

    gray_rg = expr(material, unreal.MaterialExpressionAppendVector, 900, 0)
    link(gray, gray_rg, "A")
    link(gray, gray_rg, "B")
    gray_rgb = expr(material, unreal.MaterialExpressionAppendVector, 1120, 0)
    link(gray_rg, gray_rgb, "A")
    link(gray, gray_rgb, "B")
    attributes = expr(material, unreal.MaterialExpressionMakeMaterialAttributes, 1370, 0)
    link(gray_rgb, attributes, "BaseColor")
    link(gray_rgb, attributes, "EmissiveColor")
    if not MEL.connect_material_property(attributes, "", unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES):
        raise RuntimeError("Could not connect contrast preview to Material Attributes")

    errors = MEL.recompile_material(material)
    if errors:
        raise RuntimeError("Material compile errors: " + " | ".join(str(error) for error in errors))
    if not unreal.EditorAssetLibrary.save_asset(MATERIAL_PATH):
        raise RuntimeError("Could not save " + MATERIAL_PATH)
    unreal.log("[PuddleContrastDebug] Saved {}: tile={} cm threshold={} contrast={}".format(
        MATERIAL_PATH, NOISE_TILE_CM, THRESHOLD, CONTRAST))
    unreal.log("[PuddleContrastDebug] Capture-only asset; no authored material, instance, or map was changed")


try:
    build()
except Exception:
    unreal.log_error("[PuddleContrastDebug] Creation failed:\n{}".format(traceback.format_exc()))
