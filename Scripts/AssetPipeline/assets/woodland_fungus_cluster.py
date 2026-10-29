"""A small, varied cluster of woodland mushrooms for shaded forest-floor pockets."""
import bmesh

import lib


NAME = "WoodlandFungusCluster"


def _mushroom(A, x, y, height, cap_radius, stem_radius, lean, cap_material, index):
    # A gently curved, tapering stem keeps the clump from reading as repeated pegs.
    base = (x, y, 0.004)
    shoulder = (x + lean * 0.42, y - lean * 0.12, height * 0.62)
    top = (x + lean, y, height)
    A.tube("fungus_stem", [base, shoulder, top], stem_radius,
           segs=6, taper=lambda t: 1.0 - 0.24 * t,
           name="fungusStem{:02d}".format(index))

    # Closed low dome with a soft lip. Its flat underside sits on the stem tip.
    cap_rise = cap_radius * 0.62
    profile = [
        (0.0, 0.0),
        (cap_radius * 0.84, 0.0),
        (cap_radius, cap_radius * 0.10),
        (cap_radius * 0.94, cap_radius * 0.22),
        (cap_radius * 0.76, cap_radius * 0.50),
        (cap_radius * 0.48, cap_rise),
        (0.0, cap_rise * 1.04),
    ]
    A.lathe(cap_material, profile,
            loc=(x + lean, y, height - cap_rise * 0.06),
            segs=16, name="mushroomCap{:02d}".format(index), smooth=True, sharp=45)


def build(A):
    lib.PLAIN["fungus_stem"] = {"BaseColor": [0.70, 0.61, 0.46], "Roughness": 0.88}
    lib.PLAIN["fungus_cap_ochre"] = {"BaseColor": [0.40, 0.22, 0.075], "Roughness": 0.78}
    lib.PLAIN["fungus_cap_honey"] = {"BaseColor": [0.57, 0.36, 0.13], "Roughness": 0.80}

    # An irregular, overlapping family, roughly 34 cm across and 22 cm tall.
    # Low positioning and restrained colours suit shaded groundcover, not hero props.
    mushrooms = [
        (-0.045,  0.005, 0.178, 0.070, 0.012,  0.012, "fungus_cap_ochre"),
        ( 0.112,  0.012, 0.112, 0.052, 0.009, -0.010, "fungus_cap_honey"),
        (-0.152,  0.024, 0.096, 0.046, 0.008,  0.009, "fungus_cap_honey"),
        ( 0.055, -0.119, 0.083, 0.039, 0.007, -0.006, "fungus_cap_ochre"),
        (-0.087, -0.131, 0.070, 0.034, 0.006,  0.008, "fungus_cap_honey"),
    ]
    for index, (x, y, height, cap_radius, stem_radius, lean, material) in enumerate(mushrooms, 1):
        _mushroom(A, x, y, height, cap_radius, stem_radius, lean, material, index)

    # Triangulate end caps explicitly so FBX tangents are stable across builds.
    for obj, _material in A.parts:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(obj.data)
        bm.free()

    # Deliberately no collider: this is decorative groundcover and must not alter traversal.
