"""A low wet-margin marsh-marigold clump with rounded leaves and bright spring blooms."""

import math

import bmesh
from mathutils import Vector

import lib


NAME = "TideglassMarshMarigold"
PROVENANCE = (
    "Deterministic collisionless Caltha palustris-inspired procedural geometry; "
    "low rounded leaves and glossy five-to-eight-sepal flowers for a seasonally "
    "flooded, partly open freshwater margin. Habitat and flower form: BSBI "
    "Fermanagh species account."
)
ROSETTE_CENTRES_M = (
    (-0.17, -0.12, 0.012), (0.15, -0.16, 0.012),
    (-0.14, 0.17, 0.012), (0.17, 0.15, 0.012),
)
LEAVES_PER_ROSETTE = 3
LEAF_LENGTH_M = 0.185
LEAF_WIDTH_M = 0.078
LEAF_LONGITUDINAL_SEGMENTS = 12
LEAF_LATERAL_SEGMENTS = 6
LEAF_HALF_WIDTH_PROFILE = (
    0.0, 0.43, 0.76, 0.94, 1.0, 1.035, 1.02,
    0.96, 0.86, 0.72, 0.52, 0.29, 0.0,
)
FLOWER_BASES_M = (
    (-0.09, -0.04, 0.018), (0.055, -0.08, 0.018),
    (-0.045, 0.085, 0.018), (0.105, 0.065, 0.018),
    (0.0, 0.015, 0.018),
)
FLOWER_HEIGHTS_M = (0.30, 0.36, 0.33, 0.28, 0.32)
SEPALS_PER_FLOWER = 8
SEPAL_LENGTH_M = 0.05
SEPAL_WIDTH_M = 0.023
SEPAL_LONGITUDINAL_SEGMENTS = 8
SEPAL_LATERAL_SEGMENTS = 4
SEPAL_HALF_WIDTH_PROFILE = (0.0, 0.56, 0.88, 0.99, 1.0, 0.96, 0.84, 0.62, 0.0)
SEPAL_TIP_RISE_M = 0.016
SEPAL_EDGE_CUP_M = 0.006


def _tube(bm, points, radius, segments=6):
    """Add connected low-poly stem/petiole tubes to one shared mesh."""
    points = [Vector(point) for point in points]
    rings = []
    for point_index, centre in enumerate(points):
        before = points[max(0, point_index - 1)]
        after = points[min(len(points) - 1, point_index + 1)]
        axis = (after - before).normalized()
        side = axis.cross(Vector((0.0, 0.0, 1.0)))
        if side.length_squared < 1e-8:
            side = Vector((1.0, 0.0, 0.0))
        side.normalize()
        normal = axis.cross(side).normalized()
        taper = 1.0 - 0.26 * point_index / max(1, len(points) - 1)
        ring = []
        for corner in range(segments):
            angle = math.tau * corner / segments
            offset = side * math.cos(angle) + normal * math.sin(angle)
            ring.append(bm.verts.new(centre + offset * radius * taper))
        rings.append(ring)
    for ring_index in range(len(rings) - 1):
        for corner in range(segments):
            following = (corner + 1) % segments
            bm.faces.new((rings[ring_index][corner], rings[ring_index][following],
                          rings[ring_index + 1][following], rings[ring_index + 1][corner]))
    bm.faces.new(tuple(reversed(rings[0])))
    bm.faces.new(tuple(rings[-1]))


def _leaf(bm, start, direction, scale, phase):
    """Create a rounded, shallowly heart-notched and cupped leaf blade."""
    start = Vector(start)
    direction = Vector(direction).normalized()
    across = Vector((-direction.y, direction.x, 0.0)).normalized()
    rows = []
    for row in range(LEAF_LONGITUDINAL_SEGMENTS + 1):
        t = row / LEAF_LONGITUDINAL_SEGMENTS
        centre = start + direction * (LEAF_LENGTH_M * scale * t)
        centre.z += scale * (0.016 * math.sin(math.pi * t) + 0.007 * t)
        half_width = LEAF_WIDTH_M * scale * LEAF_HALF_WIDTH_PROFILE[row]
        half_width *= 0.96 + 0.04 * math.sin(2.0 * math.pi * t + phase)
        # Split the first centimetre of the blade around its petiole, leaving a
        # small heart-shaped cleft instead of a featureless pointed paddle.
        base_notch = 0.018 * max(0.0, 1.0 - t / 0.2)
        row_vertices = []
        for column in range(LEAF_LATERAL_SEGMENTS + 1):
            across_amount = -1.0 + 2.0 * column / LEAF_LATERAL_SEGMENTS
            base_cleft = base_notch * (1.0 - abs(across_amount))
            point = centre - direction * (base_cleft * scale) + across * (half_width * across_amount)
            point.z += scale * (0.0018 * math.sin(math.pi * t) * (1.0 - abs(across_amount)))
            point.z -= scale * (0.0045 * LEAF_HALF_WIDTH_PROFILE[row] * across_amount * across_amount)
            row_vertices.append(bm.verts.new(point))
        rows.append(row_vertices)
    for row in range(len(rows) - 1):
        for side in range(LEAF_LATERAL_SEGMENTS):
            bm.faces.new((rows[row][side], rows[row + 1][side],
                          rows[row + 1][side + 1], rows[row][side + 1]))


def _sepal(bm, centre, radial, side, index):
    """Build one rounded yellow sepal with a raised tip and shallow cup."""
    rows = []
    length_scale = 0.94 + 0.06 * math.sin(index * 1.73)
    for row in range(SEPAL_LONGITUDINAL_SEGMENTS + 1):
        t = row / SEPAL_LONGITUDINAL_SEGMENTS
        along = SEPAL_LENGTH_M * length_scale * t
        half_width = SEPAL_WIDTH_M * SEPAL_HALF_WIDTH_PROFILE[row]
        tip_wave = 0.0007 * math.sin(index * 1.7 + t * math.pi)
        centre_z = 0.002 + SEPAL_TIP_RISE_M * t + 0.004 * math.sin(math.pi * t) - 0.003 * t * t + tip_wave
        mid = centre + radial * along + Vector((0.0, 0.0, centre_z))
        row_vertices = []
        for column in range(SEPAL_LATERAL_SEGMENTS + 1):
            across_amount = -1.0 + 2.0 * column / SEPAL_LATERAL_SEGMENTS
            point = mid - side * (half_width * across_amount)
            point.z -= SEPAL_EDGE_CUP_M * SEPAL_HALF_WIDTH_PROFILE[row] * across_amount * across_amount
            row_vertices.append(bm.verts.new(point))
        rows.append(row_vertices)
    for row in range(len(rows) - 1):
        for side_index in range(SEPAL_LATERAL_SEGMENTS):
            bm.faces.new((rows[row][side_index], rows[row + 1][side_index],
                          rows[row + 1][side_index + 1], rows[row][side_index + 1]))


def build(A):
    lib.PLAIN["marigold_leaf"] = {"BaseColor": [0.19, 0.32, 0.12], "Roughness": 0.74}
    lib.PLAIN["marigold_stem"] = {"BaseColor": [0.25, 0.37, 0.15], "Roughness": 0.86}
    lib.PLAIN["marigold_sepal"] = {"BaseColor": [0.98, 0.72, 0.065], "Roughness": 0.36}
    lib.PLAIN["marigold_centre"] = {"BaseColor": [0.79, 0.39, 0.035], "Roughness": 0.52}

    stems = bmesh.new()
    leaves = bmesh.new()
    for cluster_index, position in enumerate(ROSETTE_CENTRES_M):
        root = Vector(position)
        scale = (0.92, 1.0, 0.88, 0.96)[cluster_index]
        phase = 0.23 * cluster_index
        for leaf_index in range(LEAVES_PER_ROSETTE):
            angle = phase + math.tau * leaf_index / LEAVES_PER_ROSETTE + 0.17
            radial = Vector((math.cos(angle), math.sin(angle), 0.0))
            blade_start = root + radial * 0.085 * scale + Vector((0.0, 0.0, 0.052 * scale))
            petiole_mid = root + radial * 0.055 * scale + Vector((0.0, 0.0, 0.035 * scale))
            _tube(stems, (root, petiole_mid, blade_start), 0.0038 * scale)
            _leaf(leaves, blade_start, radial, scale, phase + leaf_index * 0.7)

    flower_sepal_mesh = bmesh.new()
    flower_centres = bmesh.new()
    for flower_index, (base, height) in enumerate(zip(FLOWER_BASES_M, FLOWER_HEIGHTS_M)):
        angle = math.tau * flower_index / len(FLOWER_BASES_M) + 0.28
        radial = Vector((math.cos(angle), math.sin(angle), 0.0))
        start = Vector(base)
        top = start + radial * (0.026 + 0.004 * (flower_index % 2)) + Vector((0.0, 0.0, height))
        mid = start.lerp(top, 0.54) + Vector((0.0, 0.0, 0.009))
        _tube(stems, (start, mid, top), 0.0032)
        head = top + Vector((0.0, 0.0, 0.006))
        for sepal_index in range(SEPALS_PER_FLOWER):
            petal_angle = math.tau * sepal_index / SEPALS_PER_FLOWER + flower_index * 0.19
            sepal_radial = Vector((math.cos(petal_angle), math.sin(petal_angle), 0.0))
            sepal_side = Vector((-sepal_radial.y, sepal_radial.x, 0.0))
            _sepal(flower_sepal_mesh, head, sepal_radial, sepal_side, sepal_index + flower_index)
        centre_sphere = bmesh.ops.create_uvsphere(
            flower_centres, u_segments=8, v_segments=5, radius=0.008, calc_uvs=False)
        for vert in centre_sphere["verts"]:
            vert.co += head + Vector((0.0, 0.0, 0.003))

    A._finish(stems, "marshMarigoldPetioleAndStems", "marigold_stem",
              (0, 0, 0), (0, 0, 0), "box", "Z", 1.0, True, 48)
    A._finish(leaves, "marshMarigoldLeaves", "marigold_leaf",
              (0, 0, 0), (0, 0, 0), "box", "Z", 1.0, True, 48)
    A._finish(flower_sepal_mesh, "marshMarigoldYellowSepals", "marigold_sepal",
              (0, 0, 0), (0, 0, 0), "box", "Z", 1.0, True, 48)
    A._finish(flower_centres, "marshMarigoldReceptacles", "marigold_centre",
              (0, 0, 0), (0, 0, 0), "box", "Z", 1.0, True, 48)

    # Deliberately collisionless: this asset is only a wet-edge art candidate.
    for obj, _material in A.parts:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(obj.data)
        bm.free()
