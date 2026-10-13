"""A compact, segmented russet shore crab for the Tideglass shallows."""

import bmesh
from mathutils import Vector

import lib


NAME = "TidepoolCrab"
PROVENANCE = "Geometry and flat-color materials authored in Blender 5.2 from Scripts/AssetPipeline/assets/tidepool_crab.py; no generated textures or paid model service used."


def build(A):
    # Keep this mesh self-contained: the small shore resident needs no painted
    # texture or collision and should remain cheap when several are visible.
    lib.PLAIN.update({
        "crab_shell": dict(BaseColor=[0.40, 0.125, 0.035], Roughness=0.52),
        "crab_shell_light": dict(BaseColor=[0.56, 0.245, 0.070], Roughness=0.56),
        "crab_shell_dark": dict(BaseColor=[0.23, 0.060, 0.025], Roughness=0.63),
        "crab_underbody": dict(BaseColor=[0.12, 0.055, 0.032], Roughness=0.82),
        "crab_leg": dict(BaseColor=[0.21, 0.075, 0.032], Roughness=0.72),
        "crab_eye": dict(BaseColor=[0.018, 0.024, 0.020], Roughness=0.24),
    })

    # Front points along +X. The low underside and broad domed carapace create
    # a readable crab silhouette without the old stack of scaled spheres.
    A.rock("crab_underbody", (0.255, 0.190, 0.078), loc=(-0.015, 0.0, 0.108),
           seed=17, sub=2, rough=0.045, flat_bottom=0.66, name="underside")
    A.rock("crab_shell", (0.300, 0.238, 0.128), loc=(-0.018, 0.0, 0.205),
           seed=31, sub=3, rough=0.035, flat_bottom=0.54, name="carapace")

    # A narrow, dark shell margin and a few asymmetrical warm mottles suggest a
    # naturally worn carapace rather than a flat, single-colour toy body.
    rim = A.torus("crab_shell_dark", 0.258, 0.011, loc=(-0.018, 0.0, 0.169),
                  segs=36, rsegs=6, name="carapaceRim")
    rim.scale.y = 0.79
    rim.scale.z = 0.72
    for index, (x, y, sx, sy) in enumerate((
        (-0.155, -0.045, 0.050, 0.030),
        (-0.100, 0.090, 0.038, 0.025),
        (0.015, -0.115, 0.034, 0.023),
        (0.115, 0.065, 0.030, 0.022),
    )):
        A.rock("crab_shell_light", (sx, sy, 0.009), loc=(x, y, 0.300),
               seed=47 + index, sub=1, rough=0.015, flat_bottom=0.0,
               name="shellMottle{:02d}".format(index))

    # Four pairs of jointed walking legs. Tapered two-piece forms keep the feet
    # delicate while making the body read clearly at the intended small scale.
    leg_x = (0.145, 0.035, -0.090, -0.205)
    for pair, x in enumerate(leg_x):
        sweep = (0.040, 0.012, -0.025, -0.055)[pair]
        for side in (-1, 1):
            joint = Vector((x + sweep, side * 0.305, 0.077))
            foot = Vector((x + sweep * 1.65, side * (0.385 + pair * 0.012), 0.018))
            A.tube("crab_leg", [Vector((x, side * 0.165, 0.135)),
                                 Vector((x + sweep * 0.45, side * 0.235, 0.121)),
                                 joint], 0.019, segs=7,
                   taper=lambda t: 1.0 - 0.32 * t, name="legUpper{:02d}".format(pair * 2 + (side > 0)))
            A.tube("crab_leg", [joint, Vector((foot.x - 0.012, foot.y * 0.94, 0.046)), foot],
                   0.013, segs=6, taper=lambda t: 1.0 - 0.68 * t,
                   name="legLower{:02d}".format(pair * 2 + (side > 0)))

    # Eye stalks sit forward of the shell, with dark, slightly proud compound
    # eyes; the two claws have distinct palms and opposing tapered fingers.
    for side in (-1, 1):
        A.tube("crab_leg", [Vector((0.176, side * 0.085, 0.278)),
                             Vector((0.222, side * 0.090, 0.352)),
                             Vector((0.262, side * 0.088, 0.362))],
               0.010, segs=6, taper=lambda t: 1.0 - 0.30 * t,
               name="eyeStalk{:02d}".format(side))
        A.rock("crab_eye", (0.022, 0.018, 0.020),
               loc=(0.269, side * 0.088, 0.363), seed=61 + side,
               sub=1, rough=0.0, flat_bottom=0.0, name="compoundEye{:02d}".format(side))

        A.tube("crab_leg", [Vector((0.190, side * 0.135, 0.130)),
                             Vector((0.270, side * 0.205, 0.118)),
                             Vector((0.335, side * 0.245, 0.115))],
               0.030, segs=7, taper=lambda t: 1.0 - 0.30 * t,
               name="clawArm{:02d}".format(side))
        A.rock("crab_shell", (0.088, 0.061, 0.055),
               loc=(0.355, side * 0.255, 0.108), seed=73 + side,
               sub=2, rough=0.025, flat_bottom=0.0, name="clawPalm{:02d}".format(side))
        A.tube("crab_shell_light", [Vector((0.387, side * 0.257, 0.105)),
                                    Vector((0.455, side * 0.220, 0.096)),
                                    Vector((0.480, side * 0.185, 0.081))],
               0.024, segs=6, taper=lambda t: 1.0 - 0.90 * t,
               name="clawFinger{:02d}".format(side))
        A.tube("crab_shell_dark", [Vector((0.390, side * 0.268, 0.123)),
                                   Vector((0.436, side * 0.302, 0.129)),
                                   Vector((0.457, side * 0.311, 0.118))],
               0.018, segs=6, taper=lambda t: 1.0 - 0.88 * t,
               name="clawThumb{:02d}".format(side))

    # Triangulate any n-gons before FBX tangent export; Blender otherwise warns
    # and Unreal can rebuild tangents differently from the verified preview.
    for obj, _material in A.parts:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        ngons = [face for face in bm.faces if len(face.verts) > 4]
        if ngons:
            bmesh.ops.triangulate(bm, faces=ngons, quad_method="BEAUTY", ngon_method="BEAUTY")
        bm.to_mesh(obj.data)
        bm.free()
