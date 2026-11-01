"""A small fly-agaric-inspired patch for shaded birch-and-spruce woodland pockets."""
import math

import bmesh
import lib


NAME = "WoodlandFlyAgaricCluster"

# x, y, exposed stem height, cap radius, stem radius, lean, cap material
MUSHROOMS = (
    (-0.055, 0.020, 0.132, 0.061, 0.010,  0.010, "agaric_cap_scarlet"),
    ( 0.098, 0.020, 0.095, 0.047, 0.008, -0.008, "agaric_cap_russet"),
    (-0.145, 0.025, 0.083, 0.040, 0.007,  0.008, "agaric_cap_russet"),
    ( 0.032,-0.104, 0.073, 0.035, 0.006, -0.007, "agaric_cap_scarlet"),
    (-0.095,-0.111, 0.061, 0.030, 0.005,  0.006, "agaric_cap_russet"),
)


def _cap(A, x, y, stem_top, radius, lean, material, index):
    cap_rise = radius * 0.96
    cap_x = x + lean
    cap_base = stem_top - radius * 0.025
    profile = (
        (0.0, 0.0),
        (radius * 0.78, 0.0),
        (radius * 0.97, radius * 0.08),
        (radius, radius * 0.18),
        (radius * 0.93, radius * 0.34),
        (radius * 0.78, radius * 0.57),
        (radius * 0.55, radius * 0.78),
        (radius * 0.29, radius * 0.92),
        (0.0, cap_rise),
    )
    A.lathe(material, profile, loc=(cap_x, y, cap_base), segs=18,
            name="agaricCap{:02d}".format(index), smooth=True, sharp=48)

    # Pale, low-relief veil fragments sit on two readable cap rings. Their small,
    # sparse placement keeps the silhouette clean at visitor scale.
    for ring, (fraction, surface_height, count, phase) in enumerate((
            (0.39, 0.84, 3, 0.31),
            (0.72, 0.64, 5, 0.12)), 1):
        spot_radius = radius * fraction
        for spot_index in range(count):
            angle_jitter = 0.18 * math.sin(index * 1.71 + spot_index * 2.27)
            radius_jitter = 1.0 + 0.08 * math.cos(index * 2.03 + spot_index * 1.61)
            size_jitter = 1.0 + 0.16 * math.sin(index * 1.19 + spot_index * 2.41)
            angle = (2.0 * math.pi * spot_index / count + phase
                     + index * 0.37 + angle_jitter)
            sx = cap_x + math.cos(angle) * spot_radius * radius_jitter
            sy = y + math.sin(angle) * spot_radius * radius_jitter
            sz = cap_base + radius * surface_height
            A.rock("agaric_veil", (radius * 0.16 * size_jitter,
                                    radius * 0.145 * size_jitter,
                                    radius * 0.052 * size_jitter),
                   loc=(sx, sy, sz), rot=(0.0, 0.0, math.degrees(angle)),
                   seed=index * 17 + ring * 7 + spot_index, sub=1, rough=0.08,
                   flat_bottom=0.15, name="veilFragment{:02d}".format(index))


def build(A):
    lib.PLAIN["agaric_stem"] = {"BaseColor": [0.76, 0.70, 0.57], "Roughness": 0.9}
    lib.PLAIN["agaric_cap_scarlet"] = {"BaseColor": [0.62, 0.045, 0.02], "Roughness": 0.84}
    lib.PLAIN["agaric_cap_russet"] = {"BaseColor": [0.46, 0.085, 0.03], "Roughness": 0.88}
    lib.PLAIN["agaric_veil"] = {"BaseColor": [0.92, 0.83, 0.65], "Roughness": 0.94}

    for index, (x, y, stem_height, cap_radius, stem_radius, lean, material) in enumerate(MUSHROOMS, 1):
        shoulder = (x + lean * 0.44, y - lean * 0.12, stem_height * 0.61)
        top = (x + lean, y, stem_height)
        A.tube("agaric_stem", [(x, y, 0.004), shoulder, top], stem_radius,
               segs=6, taper=lambda t: 1.0 - 0.27 * t,
               name="agaricStem{:02d}".format(index))
        _cap(A, x, y, stem_height, cap_radius, lean, material, index)

    # Stabilize FBX tangent export, including the low-poly rock-like veil flecks.
    for obj, _material in A.parts:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(obj.data)
        bm.free()

    # Decorative fungi must never change character traversal or water collision.
