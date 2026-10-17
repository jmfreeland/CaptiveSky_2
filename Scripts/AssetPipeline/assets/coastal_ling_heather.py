"""A low, branching ling-heather clump for the Island's dry coastal-heath edge."""
import math

import bmesh
from mathutils import Vector

import lib


NAME = "CoastalLingHeather"
PROVENANCE = "Collisionless procedural Calluna vulgaris-inspired geometry authored in Blender 5.2; three flat-color material slots; no generated textures."
RANDOM_SEED = 20261017
STEM_BASES = (
    (-0.105, -0.030), (-0.062, 0.088), (0.006, -0.112), (0.058, 0.021),
    (0.112, 0.074), (0.142, -0.042),
)
STEM_HEIGHTS = (0.30, 0.36, 0.28, 0.39, 0.33, 0.31)
BRANCHES_PER_STEM = 3
LEAF_NODES_PER_BRANCH = 6
LEAVES_PER_NODE = 4
FLOWERS_PER_BRANCH = 4


def _heather_leaf(bm, origin, axis, length, width, phase):
    """A tiny folded scale-leaf; the raised midrib gives it volume from oblique views."""
    direction = Vector(axis).normalized()
    side = Vector((-direction.y, direction.x, 0.0))
    if side.length_squared < 1e-6:
        side = Vector((1.0, 0.0, 0.0))
    side.normalize()
    tip = origin + direction * length
    middle = origin.lerp(tip, 0.48) + Vector((0.0, 0.0, 0.0015 * math.sin(phase)))
    left = middle + side * (width * 0.5) + Vector((0.0, 0.0, 0.0012))
    right = middle - side * (width * 0.5) - Vector((0.0, 0.0, 0.0012))
    base = bm.verts.new(origin)
    left_v = bm.verts.new(left)
    right_v = bm.verts.new(right)
    tip_v = bm.verts.new(tip)
    bm.faces.new((base, left_v, tip_v))
    bm.faces.new((left_v, right_v, tip_v))
    bm.faces.new((right_v, base, tip_v))
    bm.faces.new((base, right_v, left_v))


def _bell(bm, attachment, down, radius, length, phase):
    """A small open, four-ring bell that droops from a leafy flowering twig."""
    direction = Vector(down).normalized()
    across = Vector((0.0, 0.0, 1.0)).cross(direction)
    if across.length_squared < 1e-6:
        across = Vector((1.0, 0.0, 0.0))
    across.normalize()
    side = direction.cross(across).normalized()
    rings = ((0.18, 0.0016), (0.60, radius * 0.56), (1.0, radius))
    ring_vertices = []
    segments = 5
    for ring_index, (t, ring_radius) in enumerate(rings):
        vertices = []
        for segment in range(segments):
            angle = phase + math.tau * segment / segments
            lobe = 1.0 + (0.075 if ring_index == len(rings) - 1 else 0.0) * math.cos(4.0 * angle)
            position = attachment + direction * (length * t)
            position += (across * math.cos(angle) + side * math.sin(angle)) * ring_radius * lobe
            vertices.append(bm.verts.new(position))
        ring_vertices.append(vertices)

    crown = bm.verts.new(attachment)
    for segment in range(segments):
        bm.faces.new((crown, ring_vertices[0][segment],
                      ring_vertices[0][(segment + 1) % segments]))
    for ring_index in range(len(ring_vertices) - 1):
        for segment in range(segments):
            next_segment = (segment + 1) % segments
            bm.faces.new((ring_vertices[ring_index][segment],
                          ring_vertices[ring_index][next_segment],
                          ring_vertices[ring_index + 1][next_segment],
                          ring_vertices[ring_index + 1][segment]))


def build(A):
    lib.PLAIN["heather_stem"] = {"BaseColor": [0.22, 0.17, 0.11], "Roughness": 0.92}
    lib.PLAIN["heather_leaf"] = {"BaseColor": [0.22, 0.42, 0.18], "Roughness": 0.90}
    lib.PLAIN["heather_bloom"] = {"BaseColor": [0.49, 0.24, 0.43], "Roughness": 0.84}

    import random
    rng = random.Random(RANDOM_SEED)
    leaf_bm = bmesh.new()
    bloom_bm = bmesh.new()

    for stem_index, ((x, y), height) in enumerate(zip(STEM_BASES, STEM_HEIGHTS)):
        angle = math.tau * stem_index / len(STEM_BASES) + rng.uniform(-0.18, 0.18)
        radial = Vector((math.cos(angle), math.sin(angle), 0.0))
        base = Vector((x, y, 0.004))
        top = base + radial * rng.uniform(0.070, 0.105) + Vector((0.0, 0.0, height))
        main_path = [base.lerp(top, step / 5.0)
                     + radial * (0.052 * math.sin(math.pi * step / 5.0))
                     for step in range(6)]
        A.tube("heather_stem", main_path, 0.0042, segs=5,
               taper=lambda t: 1.0 - 0.42 * t,
               name="lingMainStem{:02d}".format(stem_index + 1))

        # Small opposing leaves also cling to the upright woody leaders.
        for node_index in range(5):
            node = base.lerp(top, 0.15 + 0.16 * node_index)
            for leaf_index in range(4):
                leaf_angle = angle + math.tau * leaf_index / 4.0 + node_index * 0.31
                leaf_radial = Vector((math.cos(leaf_angle), math.sin(leaf_angle), 0.0))
                leaf_axis = (radial * 0.25 + leaf_radial * 0.78
                             + Vector((0.0, 0.0, 0.45))).normalized()
                _heather_leaf(leaf_bm, node, leaf_axis, 0.022, 0.0090,
                              stem_index * 0.37 + leaf_index + node_index * 0.2)

        for branch_index in range(BRANCHES_PER_STEM):
            branch_t = 0.34 + 0.20 * branch_index
            branch_start = base.lerp(top, branch_t)
            side_angle = angle + (-0.72 + 0.72 * branch_index)
            branch_axis = Vector((math.cos(side_angle), math.sin(side_angle), 0.0))
            branch_length = rng.uniform(0.125, 0.165)
            branch_top = branch_start + branch_axis * branch_length + Vector((0.0, 0.0, 0.058 + 0.010 * branch_index))
            branch_path = [branch_start,
                           branch_start.lerp(branch_top, 0.50) + Vector((0.0, 0.0, 0.006)),
                           branch_top]
            A.tube("heather_stem", branch_path, 0.0025, segs=5,
                   taper=lambda t: 1.0 - 0.48 * t,
                   name="lingTwig{:02d}_{:02d}".format(stem_index + 1, branch_index + 1))

            branch_direction = (branch_top - branch_start).normalized()
            for node_index in range(LEAF_NODES_PER_BRANCH):
                t = 0.08 + 0.15 * node_index
                node = branch_start.lerp(branch_top, t)
                for leaf_index in range(LEAVES_PER_NODE):
                    leaf_angle = side_angle + math.tau * leaf_index / LEAVES_PER_NODE + 0.28 * (node_index % 2)
                    leaf_radial = Vector((math.cos(leaf_angle), math.sin(leaf_angle), 0.0))
                    leaf_axis = (branch_direction * 0.48 + leaf_radial * 0.68
                                 + Vector((0.0, 0.0, 0.38))).normalized()
                    _heather_leaf(leaf_bm, node, leaf_axis,
                                  rng.uniform(0.022, 0.028), rng.uniform(0.0090, 0.0120),
                                  stem_index * 0.41 + branch_index + leaf_index)

            # Heather carries a close row of small drooping bells near each twig tip.
            for flower_index in range(FLOWERS_PER_BRANCH):
                t = 0.43 + 0.16 * flower_index
                attachment = branch_start.lerp(branch_top, t)
                attachment += branch_axis * (0.003 * math.sin(flower_index * 1.9 + stem_index))
                down = (Vector((branch_axis.x * 0.20, branch_axis.y * 0.20, -0.98))
                        + branch_direction * 0.10).normalized()
                _bell(bloom_bm, attachment, down,
                      rng.uniform(0.0060, 0.0080), rng.uniform(0.020, 0.026),
                      stem_index * 0.63 + branch_index * 0.29 + flower_index * 0.47)

        # The strongest leaders carry a small terminal flower pair to soften their tips.
        for flower_index in range(2):
            attachment = top + radial * (0.006 + flower_index * 0.008)
            _bell(bloom_bm, attachment, Vector((radial.x * 0.15, radial.y * 0.15, -0.99)),
                  0.0070, 0.023, stem_index * 0.71 + flower_index * 0.43)

    if leaf_bm.faces:
        A._finish(leaf_bm, "lingHeatherLeaves", "heather_leaf", (0, 0, 0), (0, 0, 0),
                  "box", "Z", 1.0, True, 48)
    else:
        leaf_bm.free()

    if bloom_bm.faces:
        A._finish(bloom_bm, "lingHeatherBells", "heather_bloom", (0, 0, 0), (0, 0, 0),
                  "box", "Z", 1.0, True, 48)
    else:
        bloom_bm.free()

    # This remains lightweight, nonblocking habitat dressing, not collision or nav geometry.
    for obj, _material in A.parts:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(obj.data)
        bm.free()
