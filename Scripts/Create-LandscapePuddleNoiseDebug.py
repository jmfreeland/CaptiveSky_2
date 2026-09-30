"""Create a separate landscape-compatible raw T_LandscapeNoise preview."""

import traceback
import unreal


MATERIAL_PATH = "/Game/Materials/M_Island_PuddleNoiseRawDebug"
SOURCE_PATH = "/Game/Materials/M_Island_Textured_Auto"
TEXTURE_PATH = "/Game/Materals/Textures/T_LandscapeNoise"
NOISE_TILE_CM = 200.0

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


def build():
    if unreal.EditorAssetLibrary.does_asset_exist(MATERIAL_PATH):
        raise RuntimeError("Refusing to overwrite existing diagnostic: " + MATERIAL_PATH)

    source = unreal.load_asset(SOURCE_PATH)
    texture = unreal.load_asset(TEXTURE_PATH)
    if not source or not texture:
        raise RuntimeError("Could not load the authored landscape parent or its puddle-noise texture")

    # Preserve the valid landscape physical-material setup while overriding only the
    # visible Material Attributes output. This asset is never assigned to the map.
    material = unreal.EditorAssetLibrary.duplicate_asset(SOURCE_PATH, MATERIAL_PATH)
    if not material or material.get_class().get_name() != "Material":
        raise RuntimeError("Could not duplicate the working landscape parent")
    material.set_editor_property("use_material_attributes", True)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)

    coords = expr(material, unreal.MaterialExpressionLandscapeLayerCoords, -900, 0)
    tile_size = expr(material, unreal.MaterialExpressionScalarParameter, -900, 180,
                     parameter_name="Noise Tile Cm", default_value=NOISE_TILE_CM,
                     group="Puddle noise diagnostic")
    scaled_uv = expr(material, unreal.MaterialExpressionDivide, -650, 0)
    link(coords, scaled_uv, "A")
    link(tile_size, scaled_uv, "B")

    sample = expr(material, unreal.MaterialExpressionTextureSample, -400, 0, texture=texture)
    link(scaled_uv, sample, "UVs")
    red = expr(material, unreal.MaterialExpressionComponentMask, -150, 0,
               r=True, g=False, b=False, a=False)
    link(sample, red)

    gray_rg = expr(material, unreal.MaterialExpressionAppendVector, 100, 0)
    link(red, gray_rg, "A")
    link(red, gray_rg, "B")
    gray_rgb = expr(material, unreal.MaterialExpressionAppendVector, 330, 0)
    link(gray_rg, gray_rgb, "A")
    link(red, gray_rgb, "B")

    attributes = expr(material, unreal.MaterialExpressionMakeMaterialAttributes, 560, 0)
    link(gray_rgb, attributes, "BaseColor")
    link(gray_rgb, attributes, "EmissiveColor")
    if not MEL.connect_material_property(attributes, "", unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES):
        raise RuntimeError("Could not connect raw-noise preview to Material Attributes")

    errors = MEL.recompile_material(material)
    if errors:
        raise RuntimeError("Material compile errors: " + " | ".join(str(error) for error in errors))
    if not unreal.EditorAssetLibrary.save_asset(MATERIAL_PATH):
        raise RuntimeError("Could not save " + MATERIAL_PATH)
    unreal.log("[PuddleNoiseDebug] Saved {} with raw R-channel output and tile size {} cm".format(
        MATERIAL_PATH, NOISE_TILE_CM))
    unreal.log("[PuddleNoiseDebug] Capture-only asset; no authored material, instance, or map was changed")


try:
    build()
except Exception:
    unreal.log_error("[PuddleNoiseDebug] Creation failed:\n{}".format(traceback.format_exc()))
