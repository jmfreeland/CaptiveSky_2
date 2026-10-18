"""A broad-leaved yellow-flag iris clump for a quiet freshwater margin."""
import math

import bmesh
from mathutils import Vector

import lib


NAME = "WetlandYellowFlagIris"
PROVENANCE = (
    "Deterministic collisionless Iris pseudacorus-inspired procedural geometry authored in Blender 5.2; "
    "broad upright sword leaves, yellow iris flowers, and restrained amber signal marks; no generated textures."
)
LEAF_COUNT = 10
FLOWER_STALKS = 3
FLOWERS_PER_STALK = (1, 2, 1)
LEAF_LENGTHS = (0.48, 0.56, 0.63, 0.72, 0.78, 0.68, 0.59, 0.75, 0.54, 0.66)
LEAF_WIDTHS = (0.036, 0.041, 0.034, 0.046, 0.039, 0.044, 0.032, 0.040, 0.035, 0.043)


def _blade(bm, base, direction, length, width, bend):
    """A softly arched, closed sword leaf with a thin raised midrib."""
    base = Vector(base)
    axis = Vector(direction).normalized()
    side = Vector((-axis.y, axis.x, 0.0)).normalized()
    normal = axis.cross(side).normalized()
    rings = []
    for step in range(9):
        t = step / 8.0
        centre = base + axis * (length * t)
        centre += Vector((bend.x, bend.y, 0.0)) * math.sin(math.pi * t * 0.82)
        leaf_width = width * (0.08 + 0.92 * math.sin(math.pi * (0.04 + 0.86 * t)) ** 0.68)
        leaf_width = min(width, leaf_width)
        thickness = 0.0014 * (1.0 - 0.28 * t)
        ring = [
            bm.verts.new(centre - side * (leaf_width * 0.5) - normal * (thickness * 0.5)),
            bm.verts.new(centre + side * (leaf_width * 0.5) - normal * (thickness * 0.5)),
            bm.verts.new(centre + side * (leaf_width * 0.5) + normal * (thickness * 0.5)),
            bm.verts.new(centre - side * (leaf_width * 0.5) + normal * (thickness * 0.5)),
        ]
        rings.append(ring)
    for index in range(len(rings) - 1):
        a, b = rings[index], rings[index + 1]
        for edge in range(4):
            bm.faces.new((a[edge], a[(edge + 1) % 4], b[(edge + 1) % 4], b[edge]))
    bm.faces.new(tuple(reversed(rings[0])))
    bm.faces.new(tuple(rings[-1]))


def _ellipsoid(bm, centre, axis, side, length, width, thickness):
    """Add an oriented, low-poly, softly flattened ellipsoid."""
    centre = Vector(centre)
    axis = Vector(axis).normalized()
    side = Vector(side).normalized()
    normal = axis.cross(side).normalized()
    sphere = bmesh.ops.create_uvsphere(
        bm, u_segments=10, v_segments=6, radius=1.0, calc_uvs=False)
    for vert in sphere["verts"]:
        x, y, z = vert.co.copy()
        vert.co = centre + axis * (x * length * 0.5) + side * (y * width * 0.5) + normal * (z * thickness * 0.5)


def build(A):
    lib.PLAIN["yellow_flag_leaf"] = {"BaseColor": [0.20, 0.32, 0.13], "Roughness": 0.91}
    lib.PLAIN["yellow_flag_mid_rib"] = {"BaseColor": [0.31, 0.39, 0.16], "Roughness": 0.88}
    lib.PLAIN["yellow_flag_stem"] = {"BaseColor": [0.25, 0.34, 0.14], "Roughness": 0.9}
    lib.PLAIN["yellow_flag_petal"] = {"BaseColor": [0.82, 0.62, 0.12], "Roughness": 0.84}
    lib.PLAIN["yellow_flag_signal"] = {"BaseColor": [0.34, 0.18, 0.055], "Roughness": 0.88}

    leaves = bmesh.new()
    leaf_bases = []
    for index in range(LEAF_COUNT):
        angle = math.tau * (index / LEAF_COUNT) + 0.17 * (index % 3)
        radial = Vector((math.cos(angle), math.sin(angle), 0.0))
        base = radial * (0.035 + 0.008 * (index % 3)) + Vector((0.0, 0.0, 0.012))
        lean = 0.075 + 0.014 * (index % 4)
        direction = Vector((radial.x * lean, radial.y * lean, 1.0)).normalized()
        _blade(leaves, base, direction, LEAF_LENGTHS[index], LEAF_WIDTHS[index], radial * (0.018 + 0.004 * (index % 3)))
        leaf_bases.append((base, direction, LEAF_LENGTHS[index]))
    A._finish(leaves, "yellowFlagSwordLeaves", "yellow_flag_leaf", (0, 0, 0), (0, 0, 0), "box", "Z", 1.0, True, 48)

    for index, (base, direction, length) in enumerate(leaf_bases):
        start = base + Vector((0.0, 0.0, 0.004))
        end = base + direction * (length * 0.92)
        A.tube("yellow_flag_mid_rib", (start, start.lerp(end, 0.42), start.lerp(end, 0.78), end),
               0.0017, segs=5, taper=lambda t: 1.0 - 0.42 * t,
               name="yellowFlagMidrib{:02d}".format(index + 1))

    petals = bmesh.new()
    signals = bmesh.new()
    for stalk_index in range(FLOWER_STALKS):
        angle = math.tau * stalk_index / FLOWER_STALKS + 0.24
        radial = Vector((math.cos(angle), math.sin(angle), 0.0))
        base = radial * 0.018 + Vector((0.0, 0.0, 0.012))
        height = 0.73 + 0.055 * (stalk_index % 2)
        lean = radial * (0.035 + 0.012 * (stalk_index % 2))
        top = base + Vector((lean.x, lean.y, height))
        elbow = base.lerp(top, 0.72) + Vector((0.0, 0.0, 0.025))
        A.tube("yellow_flag_stem", (base, base.lerp(elbow, 0.42), elbow, top),
               0.0048, segs=7, taper=lambda t: 1.0 - 0.38 * t,
               name="yellowFlagFlowerStalk{:02d}".format(stalk_index + 1))

        for flower_index in range(FLOWERS_PER_STALK[stalk_index]):
            flower_centre = top - Vector((0.0, 0.0, 0.018 * flower_index))
            for petal_index in range(3):
                petal_angle = angle + math.tau * petal_index / 3.0
                flower_radial = Vector((math.cos(petal_angle), math.sin(petal_angle), 0.0))
                flower_side = Vector((-flower_radial.y, flower_radial.x, 0.0))

                standard_axis = (flower_radial * 0.26 + Vector((0.0, 0.0, 0.96))).normalized()
                _ellipsoid(petals, flower_centre + standard_axis * 0.029,
                           standard_axis, flower_side, 0.075, 0.031, 0.004)

                fall_axis = (flower_radial * 0.68 + Vector((0.0, 0.0, -0.58))).normalized()
                fall_centre = flower_centre + fall_axis * 0.033
                _ellipsoid(petals, fall_centre, fall_axis, flower_side, 0.078, 0.037, 0.0045)
                _ellipsoid(signals, fall_centre + fall_axis * 0.009 + Vector((0.0, 0.0, 0.002)),
                           fall_axis, flower_side, 0.026, 0.008, 0.0015)

            # A small golden centre keeps each bloom readable without enlarging it into a prop.
            centre_mesh = bmesh.ops.create_uvsphere(
                petals, u_segments=8, v_segments=5, radius=0.006, calc_uvs=False)
            for vert in centre_mesh["verts"]:
                vert.co += flower_centre

    A._finish(petals, "yellowFlagIrisPetals", "yellow_flag_petal", (0, 0, 0), (0, 0, 0), "box", "Z", 1.0, True, 48)
    A._finish(signals, "yellowFlagAmberSignals", "yellow_flag_signal", (0, 0, 0), (0, 0, 0), "box", "Z", 1.0, True, 48)

    # Triangulate every exported foliage surface so UE can compute stable FBX tangent space.
    for obj, _material in A.parts:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(obj.data)
        bm.free()

    # No collision is authored: this remains safe to trial as non-blocking wet-edge foliage.
