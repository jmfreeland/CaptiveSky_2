"""A loose, flowering meadowsweet clump for damp meadow edges and stream banks."""

import math

import bmesh
from mathutils import Vector

import lib


NAME = "WetlandMeadowsweet"
PROVENANCE = (
    "Deterministic collisionless Filipendula ulmaria-inspired procedural geometry; "
    "upright wiry stems, pinnate leaves, and airy cream flower clusters. Species form "
    "and habitat informed by the BSBI Fermanagh account."
)
STEM_COUNT = 5
LEAVES_PER_STEM = 2
LEAFLET_PAIRS = 4
FLOWER_BRANCHES_PER_STEM = 5
FLORETS_PER_BRANCH = 12


def _stem_tube(bm, points, radius, segments=5):
    """Build one capped, gently tapered tube along a bent plant stem."""
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
        taper = 1.0 - 0.34 * point_index / max(1, len(points) - 1)
        ring = []
        for corner in range(segments):
            angle = math.tau * corner / segments
            offset = side * math.cos(angle) + normal * math.sin(angle)
            ring.append(bm.verts.new(centre + offset * radius * taper))
        rings.append(ring)

    bm.faces.new(tuple(reversed(rings[0])))
    bm.faces.new(tuple(rings[-1]))
    for ring_index in range(len(rings) - 1):
        for corner in range(segments):
            following = (corner + 1) % segments
            bm.faces.new((rings[ring_index][corner], rings[ring_index][following],
                          rings[ring_index + 1][following], rings[ring_index + 1][corner]))


def _leaflet(bm, start, direction, length, half_width, phase):
    """Make a subtly toothed, lightly folded leaflet with real thickness."""
    start = Vector(start)
    axis = Vector(direction).normalized()
    across = Vector((-axis.y, axis.x, 0.0)).normalized()
    if across.length_squared < 1e-8:
        across = Vector((1.0, 0.0, 0.0))
    rows = []
    for step in range(9):
        t = step / 8.0
        centre = start + axis * (length * t)
        envelope = math.sin(math.pi * t) ** 0.72
        teeth = 0.94 + 0.06 * math.cos(phase + 9.0 * math.pi * t)
        width = half_width * envelope * teeth
        fold = 0.003 * envelope
        top = (
            bm.verts.new(centre - across * width + Vector((0.0, 0.0, -fold))),
            bm.verts.new(centre + Vector((0.0, 0.0, fold))),
            bm.verts.new(centre + across * width + Vector((0.0, 0.0, -fold))),
        )
        bottom = tuple(bm.verts.new(Vector(vert.co) + Vector((0.0, 0.0, -0.0014)))
                       for vert in top)
        rows.append((top, bottom))

    for row_index in range(len(rows) - 1):
        for side_index in range(2):
            a = rows[row_index][0]
            b = rows[row_index + 1][0]
            bm.faces.new((a[side_index], b[side_index], b[side_index + 1], a[side_index + 1]))
            a = rows[row_index][1]
            b = rows[row_index + 1][1]
            bm.faces.new((a[side_index + 1], b[side_index + 1], b[side_index], a[side_index]))
    bm.faces.new((rows[0][0][0], rows[0][0][1], rows[0][0][2],
                  rows[0][1][2], rows[0][1][1], rows[0][1][0]))
    bm.faces.new((rows[-1][0][2], rows[-1][0][1], rows[-1][0][0],
                  rows[-1][1][0], rows[-1][1][1], rows[-1][1][2]))


def _floret(bm, centre, radius, phase):
    """Create a tiny five-lobed flower, preserving an airy rather than solid head."""
    centre = Vector(centre)
    ring = []
    for index in range(10):
        angle = phase + math.tau * index / 10.0
        petal_tip = index % 2 == 0
        r = radius * (1.0 if petal_tip else 0.62)
        z = 0.0015 if petal_tip else 0.0
        ring.append(bm.verts.new(centre + Vector((math.cos(angle) * r,
                                                  math.sin(angle) * r, z))))
    middle = bm.verts.new(centre + Vector((0.0, 0.0, 0.001)))
    for index in range(len(ring)):
        bm.faces.new((middle, ring[index], ring[(index + 1) % len(ring)]))


def _floret_centre(bm, centre, radius):
    """Use a compact octahedron for a visible pollen centre at low cost."""
    centre = Vector(centre)
    top = bm.verts.new(centre + Vector((0.0, 0.0, radius)))
    bottom = bm.verts.new(centre - Vector((0.0, 0.0, radius)))
    ring = [
        bm.verts.new(centre + Vector((radius, 0.0, 0.0))),
        bm.verts.new(centre + Vector((0.0, radius, 0.0))),
        bm.verts.new(centre + Vector((-radius, 0.0, 0.0))),
        bm.verts.new(centre + Vector((0.0, -radius, 0.0))),
    ]
    for index in range(len(ring)):
        following = (index + 1) % len(ring)
        bm.faces.new((top, ring[index], ring[following]))
        bm.faces.new((bottom, ring[following], ring[index]))


def build(A):
    lib.PLAIN["meadowsweet_leaf"] = {"BaseColor": [0.19, 0.30, 0.105], "Roughness": 0.87}
    lib.PLAIN["meadowsweet_stem"] = {"BaseColor": [0.25, 0.34, 0.12], "Roughness": 0.88}
    lib.PLAIN["meadowsweet_flower"] = {"BaseColor": [0.91, 0.87, 0.68], "Roughness": 0.82}
    lib.PLAIN["meadowsweet_centre"] = {"BaseColor": [0.73, 0.48, 0.10], "Roughness": 0.84}

    stems = bmesh.new()
    leaves = bmesh.new()
    flowers = bmesh.new()
    centres = bmesh.new()

    for stem_index in range(STEM_COUNT):
        angle = math.tau * stem_index / STEM_COUNT + 0.18
        radial = Vector((math.cos(angle), math.sin(angle), 0.0))
        tangent = Vector((-radial.y, radial.x, 0.0))
        base = radial * (0.045 + 0.012 * (stem_index % 2))
        height = 1.02 + 0.10 * (stem_index % 3)
        lean = radial * (0.10 + 0.018 * (stem_index % 2))
        stem_path = (
            base + Vector((0.0, 0.0, 0.012)),
            base + lean * 0.27 + Vector((0.0, 0.0, height * 0.36)),
            base + lean * 0.63 + Vector((0.0, 0.0, height * 0.70)),
            base + lean + tangent * (0.018 * (stem_index % 2)) + Vector((0.0, 0.0, height)),
        )
        _stem_tube(stems, stem_path, 0.0038, segments=6)

        # Two compound leaves on each stem. Each rachis carries four opposite
        # leaflet pairs and a larger terminal leaflet, giving a recognisable
        # pinnate silhouette without a dense opaque leaf ball.
        for leaf_index in range(LEAVES_PER_STEM):
            node_t = 0.20 + 0.18 * leaf_index
            node = stem_path[0].lerp(stem_path[-1], node_t)
            rachis_length = 0.38 - 0.035 * leaf_index + 0.018 * (stem_index % 2)
            rachis_direction = (radial * 0.76 + tangent * (0.17 * (leaf_index % 2)) +
                                Vector((0.0, 0.0, 0.31))).normalized()
            rachis_tip = node + rachis_direction * rachis_length
            _stem_tube(stems, (node, node.lerp(rachis_tip, 0.52), rachis_tip), 0.0018, segments=5)

            for pair_index in range(LEAFLET_PAIRS):
                t = 0.19 + pair_index * 0.17
                attachment = node.lerp(rachis_tip, t)
                size = 1.0 - 0.095 * pair_index + 0.04 * (stem_index % 2)
                for side_sign in (-1.0, 1.0):
                    direction = (rachis_direction * 0.40 + tangent * side_sign * 0.79 +
                                 radial * 0.16 + Vector((0.0, 0.0, 0.10))).normalized()
                    start = attachment + tangent * side_sign * 0.006
                    _leaflet(leaves, start, direction, 0.115 * size, 0.030 * size,
                             phase=stem_index * 0.8 + pair_index * 0.4 + leaf_index)
            _leaflet(leaves, rachis_tip, rachis_direction, 0.135, 0.041,
                     phase=stem_index * 0.6 + leaf_index * 0.9)

        # Short, arching lateral sprays branch from the flower stem. The small
        # blooms are staggered around each spray so light and background remain
        # visible through the mature inflorescence.
        for branch_index in range(FLOWER_BRANCHES_PER_STEM):
            branch_t = 0.70 + branch_index * 0.055
            branch_base = stem_path[0].lerp(stem_path[-1], branch_t)
            branch_side = tangent * (-1.0 if branch_index % 2 else 1.0)
            branch_length = 0.095 + 0.015 * ((branch_index + stem_index) % 3)
            branch_tip = branch_base + branch_side * branch_length + radial * 0.027 + Vector((0.0, 0.0, 0.085 - branch_index * 0.008))
            branch_mid = branch_base.lerp(branch_tip, 0.62) + Vector((0.0, 0.0, 0.016))
            _stem_tube(stems, (branch_base, branch_mid, branch_tip), 0.0015, segments=5)

            for floret_index in range(FLORETS_PER_BRANCH):
                phase = stem_index * 0.7 + branch_index * 0.37 + floret_index * 2.39996322973
                spread = 0.043 * math.sqrt((floret_index + 0.5) / FLORETS_PER_BRANCH)
                floret_centre = branch_tip + branch_side * (spread * math.cos(phase))
                floret_centre += radial * (spread * math.sin(phase))
                floret_centre += Vector((0.0, 0.0, 0.018 * math.sin(phase * 1.31)))
                _floret(flowers, floret_centre, 0.008 + 0.0007 * (floret_index % 2), phase)
                _floret_centre(centres, floret_centre + Vector((0.0, 0.0, 0.0012)), 0.0018)

    A._finish(stems, "meadowsweetStemsAndRachises", "meadowsweet_stem",
              (0, 0, 0), (0, 0, 0), "box", "Z", 1.0, True, 48)
    A._finish(leaves, "meadowsweetPinnateLeaves", "meadowsweet_leaf",
              (0, 0, 0), (0, 0, 0), "box", "Z", 1.0, True, 48)
    A._finish(flowers, "meadowsweetCreamFlorets", "meadowsweet_flower",
              (0, 0, 0), (0, 0, 0), "box", "Z", 1.0, True, 48)
    A._finish(centres, "meadowsweetWarmFlowerCentres", "meadowsweet_centre",
              (0, 0, 0), (0, 0, 0), "box", "Z", 1.0, True, 48)

    for obj, _material in A.parts:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(obj.data)
        bm.free()

    # No collider: this is a plant clump, not a new navigation obstacle.
