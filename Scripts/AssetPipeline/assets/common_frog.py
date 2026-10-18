"""A small, collisionless Common Frog visual candidate for damp Tideglass margins."""

import bmesh
import math
from mathutils import Vector

import lib


NAME = "Tideglass_CommonFrog_Candidate"
PROVENANCE = (
    "Geometry and flat-color materials authored in Blender 5.2 from "
    "Scripts/AssetPipeline/assets/common_frog.py; no generated textures or paid model service used."
)


def build(A):
    # Natural brown/olive variation stays restrained, while the eye mask and
    # broad head make the forward-facing frog silhouette readable at close range.
    lib.PLAIN.update({
        "frog_back": dict(BaseColor=[0.29, 0.31, 0.17], Roughness=0.88),
        "frog_head": dict(BaseColor=[0.36, 0.36, 0.20], Roughness=0.84),
        "frog_under": dict(BaseColor=[0.48, 0.43, 0.29], Roughness=0.96),
        "frog_mask": dict(BaseColor=[0.105, 0.12, 0.07], Roughness=0.88),
        "frog_spot": dict(BaseColor=[0.18, 0.20, 0.11], Roughness=0.92),
        "frog_eye": dict(BaseColor=[0.78, 0.65, 0.25], Roughness=0.42),
        "frog_pupil": dict(BaseColor=[0.025, 0.028, 0.018], Roughness=0.3),
        "frog_limb": dict(BaseColor=[0.32, 0.32, 0.18], Roughness=0.9),
    })

    # Model is about 9 cm long, in metres, with its nose facing +X and belly
    # just above the origin. The flattened body is intentionally crouched.
    A.rock("frog_under", (0.033, 0.020, 0.010), loc=(-0.004, 0.0, 0.012),
           seed=11, sub=2, rough=0.025, flat_bottom=0.78, name="belly")
    A.rock("frog_back", (0.034, 0.022, 0.014), loc=(-0.008, 0.0, 0.023),
           seed=13, sub=3, rough=0.035, flat_bottom=0.72, name="back")
    A.rock("frog_head", (0.020, 0.020, 0.013), loc=(0.030, 0.0, 0.023),
           seed=17, sub=2, rough=0.022, flat_bottom=0.76, name="head")

    # Dark temporal masks run from each eye toward the shoulder, characteristic
    # of the common frog without painting a high-contrast cartoon stripe.
    for side in (-1, 1):
        A.rock("frog_mask", (0.018, 0.0030, 0.0040),
               loc=(0.021, side * 0.0184, 0.027), rot=(0, 0, -8),
               seed=21 + side, sub=1, rough=0.0, flat_bottom=0.0,
               name="temporalMask{:02d}".format(side))
        A.rock("frog_eye", (0.0060, 0.0055, 0.0060),
               loc=(0.032, side * 0.0145, 0.035), seed=31 + side,
               sub=2, rough=0.0, flat_bottom=0.0,
               name="eyeIris{:02d}".format(side))
        A.rock("frog_pupil", (0.0024, 0.0023, 0.0032),
               loc=(0.0345, side * 0.0173, 0.035), seed=41 + side,
               sub=1, rough=0.0, flat_bottom=0.0,
               name="eyePupil{:02d}".format(side))

    # Two understated raised eye ridges, a blunt muzzle, and irregular dorsal
    # flecks avoid the smooth single-sphere/toy look.
    for side in (-1, 1):
        A.tube("frog_head", [Vector((0.017, side * 0.011, 0.036)),
                              Vector((0.027, side * 0.016, 0.041)),
                              Vector((0.037, side * 0.012, 0.036))],
               0.0022, segs=5, taper=lambda t: 0.75 + 0.20 * t,
               name="supraocularRidge{:02d}".format(side))

    for index, (x, y, sx, sy) in enumerate((
        (-0.028, -0.009, 0.0045, 0.0030),
        (-0.020,  0.010, 0.0038, 0.0026),
        (-0.009, -0.013, 0.0034, 0.0024),
        ( 0.000,  0.009, 0.0030, 0.0022),
        (-0.036,  0.004, 0.0030, 0.0023),
    )):
        nx = (x + 0.008) / 0.034
        ny = y / 0.022
        surface_z = 0.023 + 0.014 * math.sqrt(max(0.0, 1.0 - nx * nx - ny * ny))
        A.rock("frog_spot", (sx, sy, 0.0016), loc=(x, y, surface_z + 0.00025),
               seed=51 + index, sub=1, rough=0.0, flat_bottom=0.0,
               name="dorsalMottle{:02d}".format(index))

    # Folded front legs and powerful bent hind legs. Small toe tips are grouped
    # into the single static mesh but remain separate shaped details.
    for side in (-1, 1):
        A.tube("frog_limb", [Vector((0.014, side * 0.014, 0.020)),
                              Vector((0.021, side * 0.025, 0.012)),
                              Vector((0.033, side * 0.027, 0.006))],
               0.0038, segs=6, taper=lambda t: 1.0 - 0.35 * t,
               name="frontArm{:02d}".format(side))
        A.rock("frog_limb", (0.0065, 0.0036, 0.0023),
               loc=(0.034, side * 0.028, 0.005), seed=61 + side,
               sub=1, rough=0.015, flat_bottom=0.0,
               name="frontHand{:02d}".format(side))
        for toe in range(3):
            y = side * (0.026 + toe * 0.003)
            A.tube("frog_limb", [Vector((0.035, y, 0.005)),
                                  Vector((0.039, y + side * 0.001, 0.0035))],
                   0.0009, segs=4, taper=lambda t: 1.0 - 0.65 * t,
                   name="frontToe{:02d}_{:02d}".format(side, toe))

        # Femur folded beside the body; calf returns toward the rear foot.
        A.rock("frog_limb", (0.015, 0.009, 0.006),
               loc=(-0.026, side * 0.019, 0.017), rot=(0, 0, side * 12),
               seed=71 + side, sub=2, rough=0.025, flat_bottom=0.25,
               name="hindThigh{:02d}".format(side))
        A.tube("frog_limb", [Vector((-0.030, side * 0.021, 0.018)),
                              Vector((-0.040, side * 0.029, 0.010)),
                              Vector((-0.026, side * 0.033, 0.005))],
               0.0032, segs=6, taper=lambda t: 1.0 - 0.45 * t,
               name="hindCalf{:02d}".format(side))
        A.rock("frog_limb", (0.008, 0.0042, 0.0020),
               loc=(-0.022, side * 0.034, 0.004), seed=81 + side,
               sub=1, rough=0.01, flat_bottom=0.0,
               name="hindFoot{:02d}".format(side))
        for toe in range(3):
            y = side * (0.032 + toe * 0.003)
            A.tube("frog_limb", [Vector((-0.023, y, 0.004)),
                                  Vector((-0.015 + toe * 0.002, y + side * 0.001, 0.0025))],
                   0.0010, segs=4, taper=lambda t: 1.0 - 0.68 * t,
                   name="hindToe{:02d}_{:02d}".format(side, toe))

    # Keep the body lighter underneath so a neutral preview separates belly,
    # back, and limbs without needing a textured atlas.
    A.rock("frog_under", (0.021, 0.015, 0.0022), loc=(0.001, 0.0, 0.019),
           seed=91, sub=2, rough=0.015, flat_bottom=0.0, name="ventralPatch")

    # Avoid n-gons at FBX tangent export; there are deliberately no colliders.
    for obj, _material in A.parts:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        ngons = [face for face in bm.faces if len(face.verts) > 4]
        if ngons:
            bmesh.ops.triangulate(bm, faces=ngons, quad_method="BEAUTY", ngon_method="BEAUTY")
        bm.to_mesh(obj.data)
        bm.free()
