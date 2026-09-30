"""Create a separate landscape-compatible material that visualizes MF_Puddles in grayscale."""

import traceback
import unreal


MATERIAL_PATH = "/Game/Materials/M_Island_PuddleMaskDebug_ExaggeratedRGB"
SOURCE_PATH = "/Game/Materials/M_Island_Textured_Auto"
TEXTURE_PATH = "/Game/Materals/Textures/T_LandscapeNoise"
PARAMETERS = {
    "Puddle Size": 500.0,
    "Puddle Depth": 3.0,
    "Ground Wetness": 1.0,
}
PUDDLE_CONSTRAIN = 1.0

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


def binary(material, cls, a, b, x, y):
    node = expr(material, cls, x, y)
    link(a, node, "A")
    link(b, node, "B")
    return node


def scalar(material, name, value, x, y):
    return expr(material, unreal.MaterialExpressionScalarParameter, x, y,
                parameter_name=name, default_value=value, group="Puddle mask")


def build():
    if unreal.EditorAssetLibrary.does_asset_exist(MATERIAL_PATH):
        raise RuntimeError("Refusing to overwrite existing debug material: " + MATERIAL_PATH)

    source = unreal.load_asset(SOURCE_PATH)
    if not source:
        raise RuntimeError("Could not load the working landscape parent: " + SOURCE_PATH)
    texture = unreal.load_asset(TEXTURE_PATH)
    if not texture:
        raise RuntimeError("Could not load puddle noise texture: " + TEXTURE_PATH)

    # Start from the project's functioning landscape material so its landscape-specific
    # physical-material output stays valid. Keep its Material Attributes output layout: the
    # baked landscape MICs were authored against that layout, and direct material pins did
    # compile but did not produce the diagnostic in the parent-swap capture.
    material = unreal.EditorAssetLibrary.duplicate_asset(SOURCE_PATH, MATERIAL_PATH)
    if not material or material.get_class().get_name() != "Material":
        raise RuntimeError("Could not duplicate the landscape parent as " + MATERIAL_PATH)
    material.set_editor_property("use_material_attributes", True)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)

    coords = expr(material, unreal.MaterialExpressionLandscapeLayerCoords, -1000, 0)
    scaled_uv = binary(material, unreal.MaterialExpressionDivide, coords,
                       scalar(material, "Puddle Size", PARAMETERS["Puddle Size"], -1000, 200), -700, 0)
    noise = expr(material, unreal.MaterialExpressionTextureSample, -500, 0, texture=texture)
    link(scaled_uv, noise, "UVs")
    noise_r = expr(material, unreal.MaterialExpressionComponentMask, -300, 0,
                   r=True, g=False, b=False, a=False)
    link(noise, noise_r)
    # UE 5.8 exposes Power's optional Exponent input as hidden unless explicitly enabled;
    # set it explicitly. A value of 1, larger pool scale, and full wetness exaggerate the
    # mask distribution so this capture-only diagnostic can reveal whether it reaches pixels.
    power = expr(material, unreal.MaterialExpressionPower, -100, 0, const_exponent=PUDDLE_CONSTRAIN)
    link(noise_r, power, "Base")
    constrained = power
    weighted = binary(material, unreal.MaterialExpressionMultiply, constrained,
                       scalar(material, "Puddle Depth", PARAMETERS["Puddle Depth"], -100, 250), 100, 0)
    mask = expr(material, unreal.MaterialExpressionSaturate, 300, 0)
    link(weighted, mask)
    wet_mask = binary(material, unreal.MaterialExpressionMultiply, mask,
                      scalar(material, "Ground Wetness", PARAMETERS["Ground Wetness"], 100, 250), 500, 0)

    # Normalize the mask's type first, then explicitly assemble R, G, and B. Landscape
    # expressions can carry float2 values through scalar-looking arithmetic nodes.
    mask_gray = expr(material, unreal.MaterialExpressionComponentMask, 650, 150,
                     r=True, g=False, b=False, a=False)
    link(wet_mask, mask_gray)
    mask_rg = expr(material, unreal.MaterialExpressionAppendVector, 850, 150)
    link(mask_gray, mask_rg, "A")
    link(mask_gray, mask_rg, "B")
    mask_rgb = expr(material, unreal.MaterialExpressionAppendVector, 1050, 150)
    link(mask_rg, mask_rgb, "A")
    link(mask_gray, mask_rgb, "B")

    # Emissive makes the grayscale readout independent of scene lighting; Base Color keeps
    # the same diagnostic legible in the material preview. Route through Material Attributes
    # to keep the landscape's baked material-instance layout compatible.
    attributes = expr(material, unreal.MaterialExpressionMakeMaterialAttributes, 1300, 0)
    link(mask_rgb, attributes, "BaseColor")
    link(mask_rgb, attributes, "EmissiveColor")
    if not MEL.connect_material_property(attributes, "", unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES):
        raise RuntimeError("Could not connect the diagnostic attributes to Material Attributes")

    compiler_errors = MEL.recompile_material(material)
    if compiler_errors:
        raise RuntimeError("Material compile errors: " + " | ".join(str(error) for error in compiler_errors))
    if not unreal.EditorAssetLibrary.save_asset(MATERIAL_PATH):
        raise RuntimeError("Could not save " + MATERIAL_PATH)
    unreal.log("[PuddleMaskDebug] Saved {} with diagnostic MF_Puddles values: {}, Puddle Constrain={}".format(
        MATERIAL_PATH, PARAMETERS, PUDDLE_CONSTRAIN))
    unreal.log("[PuddleMaskDebug] This separate material is a capture-only diagnostic; no map or authored material was changed")


try:
    build()
except Exception:
    unreal.log_error("[PuddleMaskDebug] Creation failed:\n{}".format(traceback.format_exc()))
