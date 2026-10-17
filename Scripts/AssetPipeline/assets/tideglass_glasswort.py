"""A compact clump of jointed annual glasswort for sheltered salt-marsh ground."""
import math
import random

import bmesh
from mathutils import Vector

import lib


NAME = "TideglassGlasswort"
BAKE_SIZE = 512


def _shoot(bm, points, radii, phase, sides=6):
    """Build a low-poly, fleshy shoot from horizontal rings and tapered joints."""
    rings = []
    for centre, radius in zip(points, radii):
        ring = []
        for index in range(sides):
            angle = phase + math.tau * index / sides
            ring.append(bm.verts.new((
                centre.x + math.cos(angle) * radius,
                centre.y + math.sin(angle) * radius,
                centre.z,
            )))
        rings.append(ring)

    for row in range(len(rings) - 1):
        for index in range(sides):
            next_index = (index + 1) % sides
            bm.faces.new((rings[row][index], rings[row][next_index],
                          rings[row + 1][next_index], rings[row + 1][index]))
    bm.faces.new(tuple(reversed(rings[0])))
    tip = bm.verts.new((points[-1].x, points[-1].y,
                        points[-1].z + radii[-1] * 0.55))
    for index in range(sides):
        bm.faces.new((rings[-1][index], rings[-1][(index + 1) % sides], tip))


def _curved_points(base, direction, height, length, fractions, bend):
    points = []
    for t in fractions:
        lateral = direction * (length * t) + bend * (math.sin(math.pi * t) * t)
        points.append(base + lateral + Vector((0.0, 0.0, height * t)))
    return points


def build(A):
    lib.PLAIN["glasswort"] = {"BaseColor": [0.18, 0.36, 0.16], "Roughness": 0.88}
    lib.PLAIN["glasswort_mature"] = {"BaseColor": [0.46, 0.23, 0.12], "Roughness": 0.84}

    rng = random.Random(83017)
    green_bm = bmesh.new()
    mature_bm = bmesh.new()
    fractions = (0.0, 0.17, 0.36, 0.56, 0.78, 1.0)
    shoot_count = 10

    # Upright jointed shoots form a low, open pioneer-marsh cushion; no leaves
    # or flowers are added because Salicornia's fleshy stem joints are its read.
    for index in range(shoot_count):
        angle = math.tau * index / shoot_count + rng.uniform(-0.24, 0.24)
        radial = Vector((math.cos(angle), math.sin(angle), 0.0))
        base_radius = rng.uniform(0.025, 0.15)
        base = radial * base_radius + Vector((0.0, 0.0, 0.006))
        lateral_bend = Vector((-radial.y, radial.x, 0.0)) * rng.uniform(-0.016, 0.016)
        height = rng.uniform(0.19, 0.28)
        length = rng.uniform(0.025, 0.065)
        points = _curved_points(base, radial, height, length, fractions, lateral_bend)
        stem_radius = rng.uniform(0.009, 0.013)
        joint_radii = (stem_radius * 0.68, stem_radius * 0.92,
                       stem_radius * 0.72, stem_radius,
                       stem_radius * 0.74, stem_radius * 0.76)
        _shoot(green_bm, points, joint_radii, angle)

        # Two small side forks emerge from separate middle joints, widening the
        # clump without turning its outline into grass blades or a flower head.
        for branch_index, node_index in enumerate((2, 3)):
            branch_angle = angle + (1.0 if (index + branch_index) % 2 == 0 else -1.0)
            branch_direction = Vector((math.cos(branch_angle), math.sin(branch_angle), 0.0))
            branch_length = rng.uniform(0.045, 0.095)
            branch_height = rng.uniform(0.045, 0.085)
            branch_base = points[node_index]
            branch_points = _curved_points(
                branch_base, branch_direction, branch_height, branch_length,
                (0.0, 0.34, 0.72, 1.0), branch_direction * 0.012,
            )
            branch_radius = stem_radius * rng.uniform(0.48, 0.7)
            _shoot(green_bm, branch_points,
                   (branch_radius * 0.72, branch_radius, branch_radius * 0.78, branch_radius * 0.68),
                   branch_angle)

        # A few late-season shoot tips bronze naturally; this is a static art
        # variant, not a claim that a seasonal simulation currently exists.
        if index in (2, 7, 9):
            tip_base = points[-1]
            tip_direction = radial * 0.55 + Vector((0.0, 0.0, 0.84))
            tip_points = [tip_base, tip_base + tip_direction * 0.022, tip_base + tip_direction * 0.04]
            tip_radius = stem_radius * 0.22
            _shoot(mature_bm, tip_points, (tip_radius, tip_radius * 0.72, tip_radius * 0.12), angle)

    if green_bm.faces:
        A._finish(green_bm, "glasswortShoots", "glasswort", (0, 0, 0), (0, 0, 0),
                  "box", "Z", 1.0, True, 48)
    else:
        green_bm.free()
    if mature_bm.faces:
        A._finish(mature_bm, "glasswortMatureTips", "glasswort_mature", (0, 0, 0), (0, 0, 0),
                  "box", "Z", 1.0, True, 48)
    else:
        mature_bm.free()

    # Keep FBX tangents and the triangle budget deterministic.
    for obj, _material in A.parts:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(obj.data)
        bm.free()
