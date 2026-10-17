"""A compact cross-leaved heath clump for damp, acidic heath-to-mire edges."""
import math
import random

import bmesh
from mathutils import Vector

import lib


NAME = "CoastalCrossLeavedHeath"
PROVENANCE = "Collisionless procedural Erica tetralix-inspired geometry authored in Blender 5.2; three flat-color material slots; no generated textures."
RANDOM_SEED = 20261018
LEADER_BASES = (
    (-0.13, -0.08), (-0.08, 0.11), (0.005, -0.12), (0.04, 0.015),
    (0.12, 0.09), (0.16, -0.035), (-0.015, 0.16),
)
LEADER_HEIGHTS = (0.32, 0.36, 0.31, 0.38, 0.34, 0.32, 0.33)
LEAVES_PER_WHORL = 4
LEADER_WHORLS = 6
SIDE_BRANCHES_PER_LEADER = 2
SIDE_WHORLS_PER_BRANCH = 2
BELLS_PER_CLUSTER = 3
BELL_SEGMENTS = 5


def _leaf(bm, origin, axis, length, width, phase):
    """A small evergreen leaf with a raised midrib and a solid back."""
    direction = Vector(axis).normalized()
    across = Vector((-direction.y, direction.x, 0.0))
    if across.length_squared < 1e-6:
        across = Vector((1.0, 0.0, 0.0))
    across.normalize()
    tip = origin + direction * length
    mid = origin.lerp(tip, 0.54)
    left = mid + across * (width * 0.5) + Vector((0.0, 0.0, 0.0010 * math.sin(phase) + 0.0007))
    right = mid - across * (width * 0.5) - Vector((0.0, 0.0, 0.0010 * math.sin(phase) + 0.0007))
    base_v, left_v, tip_v, right_v = [bm.verts.new(point) for point in (origin, left, tip, right)]
    # A tetrahedral low-poly lanceolate leaf gives both-sided shading in four faces.
    bm.faces.new((base_v, left_v, tip_v))
    bm.faces.new((tip_v, right_v, base_v))
    bm.faces.new((base_v, right_v, left_v))
    bm.faces.new((left_v, right_v, tip_v))


def _tube_segment(bm, start, end, radius, phase=0.0, sides=5):
    """Add a tiny tapered pedicel into one shared mesh, avoiding per-flower objects."""
    axis = (end - start).normalized()
    reference = Vector((0.0, 0.0, 1.0)) if abs(axis.z) < 0.9 else Vector((1.0, 0.0, 0.0))
    across = axis.cross(reference).normalized()
    side = axis.cross(across).normalized()
    rings = []
    for centre, scale in ((start, 1.0), (end, 0.68)):
        ring = []
        for index in range(sides):
            angle = phase + math.tau * index / sides
            point = centre + (across * math.cos(angle) + side * math.sin(angle)) * radius * scale
            ring.append(bm.verts.new(point))
        rings.append(ring)
    for index in range(sides):
        nxt = (index + 1) % sides
        bm.faces.new((rings[0][index], rings[0][nxt], rings[1][nxt], rings[1][index]))


def _bell(bm, attachment, down, radius, length, phase):
    """A small urn-shaped, drooping Erica bell with a flared five-lobed mouth."""
    direction = Vector(down).normalized()
    across = Vector((0.0, 0.0, 1.0)).cross(direction)
    if across.length_squared < 1e-6:
        across = Vector((1.0, 0.0, 0.0))
    across.normalize()
    side = direction.cross(across).normalized()
    rings = ((0.0, radius * 0.38), (0.14, radius * 0.68),
             (0.48, radius), (0.80, radius * 0.94), (1.0, radius * 0.88))
    ring_vertices = []
    for ring_index, (t, ring_radius) in enumerate(rings):
        vertices = []
        for segment in range(BELL_SEGMENTS):
            angle = phase + math.tau * segment / BELL_SEGMENTS
            lobe = 1.0 + (0.10 if ring_index == len(rings) - 1 else 0.025) * math.cos(5.0 * angle)
            point = attachment + direction * (length * t)
            point += (across * math.cos(angle) + side * math.sin(angle)) * ring_radius * lobe
            vertices.append(bm.verts.new(point))
        ring_vertices.append(vertices)
    for ring_index in range(len(ring_vertices) - 1):
        for segment in range(BELL_SEGMENTS):
            nxt = (segment + 1) % BELL_SEGMENTS
            bm.faces.new((ring_vertices[ring_index][segment], ring_vertices[ring_index][nxt],
                          ring_vertices[ring_index + 1][nxt], ring_vertices[ring_index + 1][segment]))


def _add_whorl(leaf_bm, centre, axis, phase, rng, size=1.0):
    """Four short leaves radiating around a leader, the defining cross-leaved habit."""
    axis = Vector(axis).normalized()
    for leaf_index in range(LEAVES_PER_WHORL):
        angle = phase + math.tau * leaf_index / LEAVES_PER_WHORL
        radial = Vector((math.cos(angle), math.sin(angle), 0.0))
        leaf_axis = (radial * 0.83 + axis * 0.22 + Vector((0.0, 0.0, 0.18))).normalized()
        _leaf(leaf_bm, centre, leaf_axis,
              rng.uniform(0.038, 0.044) * size,
              rng.uniform(0.0090, 0.0105) * size,
              phase + leaf_index * 0.51)


def _add_flower_cluster(stem_bm, bloom_bm, anchor, axis, phase, rng):
    """Cluster five pendant bells around a shoot tip on short shared-mesh pedicels."""
    axis = Vector(axis).normalized()
    for flower_index in range(BELLS_PER_CLUSTER):
        angle = phase + math.tau * flower_index / BELLS_PER_CLUSTER
        radial = Vector((math.cos(angle), math.sin(angle), 0.0))
        attachment = anchor + radial * rng.uniform(0.009, 0.016)
        attachment += Vector((0.0, 0.0, rng.uniform(-0.002, 0.009)))
        _tube_segment(stem_bm, anchor, attachment, 0.0010, angle)
        down = (radial * 0.22 + axis * 0.12 + Vector((0.0, 0.0, -0.97))).normalized()
        _bell(bloom_bm, attachment, down,
              rng.uniform(0.0053, 0.0064), rng.uniform(0.017, 0.021), angle * 0.7)


def build(A):
    lib.PLAIN["cross_heath_stem"] = {"BaseColor": [0.22, 0.20, 0.12], "Roughness": 0.92}
    lib.PLAIN["cross_heath_leaf"] = {"BaseColor": [0.24, 0.43, 0.18], "Roughness": 0.88}
    lib.PLAIN["cross_heath_bloom"] = {"BaseColor": [0.72, 0.36, 0.50], "Roughness": 0.82}

    rng = random.Random(RANDOM_SEED)
    leaf_bm = bmesh.new()
    stem_bm = bmesh.new()
    bloom_bm = bmesh.new()

    for leader_index, ((x, y), height) in enumerate(zip(LEADER_BASES, LEADER_HEIGHTS)):
        phase = math.tau * leader_index / len(LEADER_BASES) + rng.uniform(-0.18, 0.18)
        radial = Vector((math.cos(phase), math.sin(phase), 0.0))
        base = Vector((x, y, 0.004))
        top = base + radial * rng.uniform(0.060, 0.095) + Vector((0.0, 0.0, height))
        leader_path = [base.lerp(top, step / 5.0)
                       + radial * (0.022 * math.sin(math.pi * step / 5.0))
                       for step in range(6)]
        A.tube("cross_heath_stem", leader_path, 0.0038, segs=5,
               taper=lambda t: 1.0 - 0.40 * t,
               name="crossHeathLeader{:02d}".format(leader_index + 1))

        for whorl_index in range(LEADER_WHORLS):
            t = 0.10 + 0.11 * whorl_index
            centre = base.lerp(top, t)
            _add_whorl(leaf_bm, centre, radial, phase + whorl_index * 0.43, rng)

        leader_direction = (top - base).normalized()
        _add_flower_cluster(stem_bm, bloom_bm, top, leader_direction, phase, rng)

        for branch_index in range(SIDE_BRANCHES_PER_LEADER):
            branch_t = 0.35 + 0.18 * branch_index
            branch_start = base.lerp(top, branch_t)
            branch_sign = -1.0 if (leader_index + branch_index) % 2 == 0 else 1.0
            branch_angle = phase + branch_sign * (0.95 + rng.uniform(-0.12, 0.12))
            branch_radial = Vector((math.cos(branch_angle), math.sin(branch_angle), 0.0))
            branch_top = branch_start + branch_radial * rng.uniform(0.14, 0.18)
            branch_top += Vector((0.0, 0.0, rng.uniform(0.035, 0.058)))
            branch_path = [branch_start,
                           branch_start.lerp(branch_top, 0.5) + Vector((0.0, 0.0, 0.006)),
                           branch_top]
            A.tube("cross_heath_stem", branch_path, 0.0024, segs=5,
                   taper=lambda t: 1.0 - 0.48 * t,
                   name="crossHeathBranch{:02d}_{:02d}".format(leader_index + 1, branch_index + 1))

            branch_direction = (branch_top - branch_start).normalized()
            for whorl_index in range(SIDE_WHORLS_PER_BRANCH):
                t = 0.16 + 0.32 * whorl_index
                centre = branch_start.lerp(branch_top, t)
                _add_whorl(leaf_bm, centre, branch_direction,
                           branch_angle + whorl_index * 0.58, rng, size=0.92)
            _add_flower_cluster(stem_bm, bloom_bm, branch_top, branch_direction,
                                branch_angle, rng)

    if leaf_bm.faces:
        A._finish(leaf_bm, "crossHeathWhorledLeaves", "cross_heath_leaf", (0, 0, 0),
                  (0, 0, 0), "box", "Z", 1.0, True, 48)
    else:
        leaf_bm.free()

    if stem_bm.faces:
        A._finish(stem_bm, "crossHeathFlowerPedicels", "cross_heath_stem", (0, 0, 0),
                  (0, 0, 0), "box", "Z", 1.0, True, 48)
    else:
        stem_bm.free()

    if bloom_bm.faces:
        A._finish(bloom_bm, "crossHeathUrnBells", "cross_heath_bloom", (0, 0, 0),
                  (0, 0, 0), "box", "Z", 1.0, True, 48)
    else:
        bloom_bm.free()

    # This species dresses wet-heath transitions only; it has no collision/navigation role.
    for obj, _material in A.parts:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(obj.data)
        bm.free()
