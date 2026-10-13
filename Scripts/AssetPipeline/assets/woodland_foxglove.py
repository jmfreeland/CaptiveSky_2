"""A small woodland-edge foxglove colony with upright flower spires and basal leaves."""
import math

import bmesh
from mathutils import Vector

import lib


NAME = "WoodlandFoxglove"
SPIKE_HEIGHTS = (1.28, 1.14, 1.04)
FLOWERS_PER_SPIKE = 8
ROSETTE_LEAVES_PER_PLANT = 5
PLANT_BASES = ((-0.11, -0.07, 0.01), (0.18, 0.08, 0.01), (-0.03, 0.22, 0.01))


def _leaf(leaf_bm, origin, angle, length, width, phase):
    direction = Vector((math.cos(angle), math.sin(angle), 0.0))
    across = Vector((-direction.y, direction.x, 0.0))
    rows = []
    for row in range(9):
        t = row / 8
        centre = origin + direction * (length * t)
        centre.z += 0.19 * t + 0.025 * math.sin(math.pi * t + phase)
        profile = math.sin(math.pi * t) ** 0.78
        half_width = width * profile * (0.55 + 0.45 * (1.0 - t))
        edge_scale = 0.94 if row in (2, 5, 7) else 1.0
        verts = []
        for side in (-1.0, 0.0, 1.0):
            point = centre + across * (side * half_width * edge_scale)
            point.z += 0.009 * (1.0 - side * side) * math.sin(math.pi * t)
            verts.append(leaf_bm.verts.new(point))
        rows.append(verts)
    for row in range(8):
        for col in range(2):
            leaf_bm.faces.new((rows[row][col], rows[row + 1][col],
                               rows[row + 1][col + 1], rows[row][col + 1]))


def _bell(bloom_bm, attachment, axis, radius, length, phase):
    axis = axis.normalized()
    reference = Vector((0.0, 0.0, 1.0))
    across = reference.cross(axis)
    if across.length_squared < 1e-6:
        across = Vector((1.0, 0.0, 0.0))
    across.normalize()
    side = axis.cross(across).normalized()
    rings = ((0.0, 0.0055), (0.16, 0.007), (0.40, radius * 0.49),
             (0.68, radius * 0.78), (0.90, radius), (1.0, radius * 0.91))
    ring_verts = []
    count = 8
    for t, ring_radius in rings:
        ring = []
        for i in range(count):
            angle = math.tau * i / count + phase
            lobing = 1.0 + 0.12 * max(0.0, t - 0.72) * math.cos(5.0 * angle + phase)
            point = attachment + axis * (length * t)
            point += (across * math.cos(angle) + side * math.sin(angle)) * ring_radius * lobing
            ring.append(bloom_bm.verts.new(point))
        ring_verts.append(ring)
    for row in range(len(ring_verts) - 1):
        for i in range(count):
            j = (i + 1) % count
            bloom_bm.faces.new((ring_verts[row][i], ring_verts[row][j],
                                ring_verts[row + 1][j], ring_verts[row + 1][i]))


def _throat_marks(mark_bm, attachment, axis, length, phase):
    axis = axis.normalized()
    across = Vector((0.0, 0.0, 1.0)).cross(axis)
    if across.length_squared < 1e-6:
        across = Vector((1.0, 0.0, 0.0))
    across.normalize()
    side = axis.cross(across).normalized()
    centre = attachment + axis * (length * 0.67)
    # Three tiny inner marks give the open bells a readable foxglove throat.
    for index, (u, v) in enumerate(((-0.006, 0.0), (0.006, 0.001), (0.0, -0.006))):
        point = centre + across * (u + 0.002 * math.sin(phase + index)) + side * v
        half = 0.0032
        corners = (point + across * half, point + side * half,
                   point - across * half, point - side * half)
        mark_bm.faces.new(tuple(mark_bm.verts.new(corner) for corner in corners))


def build(A):
    lib.PLAIN["foxglove_leaf"] = {"BaseColor": [0.12, 0.25, 0.09], "Roughness": 0.88}
    lib.PLAIN["foxglove_stem"] = {"BaseColor": [0.20, 0.31, 0.12], "Roughness": 0.86}
    lib.PLAIN["foxglove_bloom"] = {"BaseColor": [0.53, 0.22, 0.48], "Roughness": 0.76}
    lib.PLAIN["foxglove_throat"] = {"BaseColor": [0.19, 0.065, 0.20], "Roughness": 0.82}

    leaf_bm = bmesh.new()
    bloom_bm = bmesh.new()
    mark_bm = bmesh.new()

    for plant_index, (base_tuple, height) in enumerate(zip(PLANT_BASES, SPIKE_HEIGHTS)):
        base = Vector(base_tuple)
        phase = plant_index * 2.1 + 0.45
        lean = Vector((math.cos(phase), math.sin(phase), 0.0)) * (0.055 + 0.018 * plant_index)
        top = base + lean + Vector((0.0, 0.0, height))
        path = [base.lerp(top, step / 7) + Vector((0.0, 0.0, 0.012 * math.sin(math.pi * step / 7)))
                for step in range(8)]
        A.tube("foxglove_stem", path, 0.0065, segs=6,
               taper=lambda t: 1.0 - 0.36 * t, name="foxgloveSpike{:02d}".format(plant_index + 1))

        # Five long, low leaves form a broken rosette rather than a solid mound.
        for leaf_index in range(ROSETTE_LEAVES_PER_PLANT):
            angle = phase + math.tau * leaf_index / ROSETTE_LEAVES_PER_PLANT
            origin = base + Vector((0.0, 0.0, 0.012 + 0.003 * (leaf_index % 2)))
            length = 0.48 + 0.055 * ((leaf_index + plant_index) % 3)
            width = 0.12 + 0.015 * ((leaf_index + 2 * plant_index) % 3)
            _leaf(leaf_bm, origin, angle, length, width, phase + leaf_index * 0.6)

        # The pendant, outward-facing bells alternate sides along the upper raceme.
        for flower_index in range(FLOWERS_PER_SPIKE):
            t = 0.48 + 0.061 * flower_index
            side_angle = phase + (0.0 if flower_index % 2 == 0 else math.pi) + 0.12 * flower_index
            radial = Vector((math.cos(side_angle), math.sin(side_angle), 0.0))
            attachment = base.lerp(top, t) + radial * 0.014
            start = base.lerp(top, t)
            A.tube("foxglove_stem", [start, start.lerp(attachment, 0.55), attachment],
                   0.0025, segs=4, taper=lambda u: 1.0 - 0.28 * u,
                   name="foxglovePedicel{:02d}_{:02d}".format(plant_index + 1, flower_index + 1))
            axis = (radial * 0.59 + Vector((0.0, 0.0, -0.81)))
            length = 0.102 + 0.006 * ((flower_index + plant_index) % 3)
            radius = 0.030 + 0.002 * ((flower_index + 2 * plant_index) % 2)
            _bell(bloom_bm, attachment, axis, radius, length, phase + flower_index * 0.37)
            _throat_marks(mark_bm, attachment, axis, length, phase + flower_index * 0.37)

    if leaf_bm.faces:
        bmesh.ops.solidify(leaf_bm, geom=leaf_bm.faces[:], thickness=0.0018)
        A._finish(leaf_bm, "foxgloveRosette", "foxglove_leaf", (0, 0, 0), (0, 0, 0),
                  "box", "Z", 1.0, True, 50)
    else:
        leaf_bm.free()

    if bloom_bm.faces:
        bmesh.ops.solidify(bloom_bm, geom=bloom_bm.faces[:], thickness=0.0016)
        A._finish(bloom_bm, "foxgloveBells", "foxglove_bloom", (0, 0, 0), (0, 0, 0),
                  "box", "Z", 1.0, True, 48)
    else:
        bloom_bm.free()

    if mark_bm.faces:
        bmesh.ops.solidify(mark_bm, geom=mark_bm.faces[:], thickness=0.0008)
        A._finish(mark_bm, "foxgloveThroats", "foxglove_throat", (0, 0, 0), (0, 0, 0),
                  "box", "Z", 1.0, True, 48)
    else:
        mark_bm.free()

    # Keep exports deterministic and avoid relying on FBX triangulation heuristics.
    for obj, _material in A.parts:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(obj.data)
        bm.free()
