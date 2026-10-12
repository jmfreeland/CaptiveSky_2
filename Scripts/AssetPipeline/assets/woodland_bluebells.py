"""A loose clump of drooping woodland bluebells with long, folded leaves."""
import math
import random

import bmesh
from mathutils import Vector

import lib


NAME = "WoodlandBluebells"


def _leaf(leaf_bm, origin, angle, length, width):
    direction = Vector((math.cos(angle), math.sin(angle), 0.0))
    across = Vector((-direction.y, direction.x, 0.0))
    rows = []
    for i in range(7):
        t = i / 6
        centre = origin + direction * (length * t)
        centre.z += 0.012 * math.sin(math.pi * t)
        half_width = width * (math.sin(math.pi * t) ** 0.72) * (1.0 - 0.28 * t)
        row = []
        for side in (-1.0, 0.0, 1.0):
            point = centre + across * (side * half_width)
            point.z += 0.006 * (1.0 - side * side) * math.sin(math.pi * t)
            row.append(leaf_bm.verts.new(point))
        rows.append(row)
    for row in range(6):
        for col in range(2):
            leaf_bm.faces.new((rows[row][col], rows[row + 1][col],
                               rows[row + 1][col + 1], rows[row][col + 1]))


def _bell(bloom_bm, attachment, down, radius, length, phase):
    down = down.normalized()
    across = Vector((0.0, 0.0, 1.0)).cross(down).normalized()
    side = down.cross(across).normalized()
    # The narrow neck sits at the stem; the flared, five-lobed mouth points down.
    rings = ((0.0, 0.005), (0.18, 0.008), (0.42, radius * 0.54),
             (0.70, radius * 0.84), (0.90, radius), (1.0, radius * 0.91))
    ring_verts = []
    count = 8
    for t, ring_radius in rings:
        ring = []
        for i in range(count):
            angle = math.tau * i / count + phase
            lobe = 1.0 + (0.11 * max(0.0, t - 0.68)) * math.cos(5.0 * angle + phase)
            p = attachment + down * (length * t)
            p += (across * math.cos(angle) + side * math.sin(angle)) * ring_radius * lobe
            ring.append(bloom_bm.verts.new(p))
        ring_verts.append(ring)
    for row in range(len(ring_verts) - 1):
        for i in range(count):
            j = (i + 1) % count
            bloom_bm.faces.new((ring_verts[row][i], ring_verts[row][j],
                                ring_verts[row + 1][j], ring_verts[row + 1][i]))


def build(A):
    lib.PLAIN["bluebell_leaf"] = {"BaseColor": [0.11, 0.23, 0.075], "Roughness": 0.86}
    lib.PLAIN["bluebell_stem"] = {"BaseColor": [0.20, 0.31, 0.11], "Roughness": 0.82}
    lib.PLAIN["bluebell_bloom"] = {"BaseColor": [0.24, 0.20, 0.52], "Roughness": 0.76}

    rng = random.Random(62041)
    leaf_bm = bmesh.new()
    bloom_bm = bmesh.new()

    # Nine slender stems make a loose, irregular colony rather than a dense ball.
    stems = []
    for index in range(7):
        angle = math.tau * index / 7 + rng.uniform(-0.22, 0.22)
        radius = rng.uniform(0.04, 0.28)
        base = Vector((math.cos(angle) * radius, math.sin(angle) * radius, 0.006))
        height = rng.uniform(0.25, 0.36)
        lean = Vector((math.cos(angle + 1.2), math.sin(angle + 1.2), 0.0)) * rng.uniform(0.025, 0.065)
        top = base + lean + Vector((0.0, 0.0, height))
        points = [base.lerp(top, t / 5) + Vector((0, 0, 0.008 * math.sin(math.pi * t / 5)))
                  for t in range(6)]
        A.tube("bluebell_stem", points, 0.0035, segs=5,
               taper=lambda t: 1.0 - 0.30 * t, name=f"bluebellStem{index + 1:02d}")
        stems.append((base, top, angle, height))

        # Two or three strap leaves lie low and sweep away from each stem.
        for leaf_index in range(2):
            leaf_angle = angle + math.pi + (leaf_index - 0.5) * 1.08 + rng.uniform(-0.18, 0.18)
            leaf_length = rng.uniform(0.24, 0.38)
            _leaf(leaf_bm, base + Vector((0.0, 0.0, 0.012)), leaf_angle,
                  leaf_length, rng.uniform(0.018, 0.026))

        # Pendulous flowers alternate along the upper stem, with individual droop.
        flower_count = 2 + (index % 3 == 0)
        for flower_index in range(flower_count):
            t = 0.62 + 0.31 * (flower_index / max(1, flower_count - 1))
            attachment = base.lerp(top, t)
            radial = Vector((math.cos(angle + flower_index * 1.7),
                             math.sin(angle + flower_index * 1.7), 0.0))
            attachment += radial * 0.026
            attachment += Vector((0.0, 0.0, 0.012 * math.sin(flower_index * 1.7 + index)))
            branch_start = base.lerp(top, t)
            A.tube("bluebell_stem", [branch_start, branch_start.lerp(attachment, 0.55), attachment],
                   0.0022, segs=4, taper=lambda u: 1.0 - 0.34 * u,
                   name=f"bluebellPedicel{index + 1:02d}_{flower_index + 1:02d}")
            tangent = (lean + Vector((0.0, 0.0, height))).normalized()
            down = (tangent * 0.30 + Vector((0.0, 0.0, -0.94)) + radial * 0.22)
            _bell(bloom_bm, attachment, down, rng.uniform(0.021, 0.028),
                  rng.uniform(0.043, 0.057), rng.uniform(0.0, math.tau))

    if leaf_bm.faces:
        bmesh.ops.solidify(leaf_bm, geom=leaf_bm.faces[:], thickness=0.0012)
        A._finish(leaf_bm, "bluebellLeaves", "bluebell_leaf", (0, 0, 0), (0, 0, 0),
                  "box", "Z", 1.0, True, 48)
    else:
        leaf_bm.free()

    if bloom_bm.faces:
        bmesh.ops.solidify(bloom_bm, geom=bloom_bm.faces[:], thickness=0.0015)
        A._finish(bloom_bm, "bluebellBells", "bluebell_bloom", (0, 0, 0), (0, 0, 0),
                  "box", "Z", 1.0, True, 48)
    else:
        bloom_bm.free()

    # Export stable triangulated meshes so FBX tangent generation is repeatable.
    for obj, _material in A.parts:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(obj.data)
        bm.free()
