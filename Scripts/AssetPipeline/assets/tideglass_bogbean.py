"""A low bogbean mat with trifoliate leaves and fringed white flowers."""

import math

import bmesh
from mathutils import Vector

import lib


NAME = "TideglassBogbean"
PROVENANCE = (
    "Deterministic collisionless Menyanthes trifoliata-inspired procedural geometry; "
    "creeping wet-margin habit, trifoliate leaves, and pale fringed flowers. "
    "Habitat reference: British and Irish Botany (BSBI), Bogbean account."
)
LEAF_CLUSTERS = 6
FLOWER_STALKS = 3
FLOWERS_PER_STALK = (4, 5, 4)
FLOWER_RACEME_START_T = 0.56
FLOWER_RACEME_END_T = 0.96
LEAFLET_COUNT_PER_CLUSTER = 3
FLOWER_PETAL_COUNT = 5
FLOWER_HEIGHTS_M = (0.39, 0.44, 0.41)
LEAFLET_LENGTH_M = 0.105
LEAFLET_WIDTH_M = 0.060
FLOWER_DIAMETER_M = 0.058


def _oriented_ellipsoid(bm, centre, axis, side, length, width, thickness, segments=8):
    """Add a small, oriented ellipsoid without external textures or collision."""
    centre = Vector(centre)
    axis = Vector(axis).normalized()
    side = Vector(side).normalized()
    normal = axis.cross(side).normalized()
    sphere = bmesh.ops.create_uvsphere(
        bm, u_segments=segments, v_segments=max(4, segments // 2),
        radius=1.0, calc_uvs=False)
    for vert in sphere["verts"]:
        x, y, z = vert.co.copy()
        vert.co = (
            centre
            + axis * (x * length * 0.5)
            + side * (y * width * 0.5)
            + normal * (z * thickness * 0.5)
        )


def _fringe_filament(bm, start, end, radius=0.00038):
    """Add a triangular tapered rod, keeping many tiny flower hairs cheap."""
    start = Vector(start)
    end = Vector(end)
    axis = (end - start).normalized()
    side = Vector((-axis.y, axis.x, 0.0))
    if side.length_squared < 1e-8:
        side = Vector((1.0, 0.0, 0.0))
    side.normalize()
    normal = axis.cross(side).normalized()
    rings = []
    for centre, ring_radius in ((start, radius), (end, radius * 0.18)):
        ring = []
        for corner in range(3):
            angle = math.tau * corner / 3.0
            ring.append(bm.verts.new(
                centre + side * (math.cos(angle) * ring_radius)
                + normal * (math.sin(angle) * ring_radius)))
        rings.append(ring)
    for corner in range(3):
        next_corner = (corner + 1) % 3
        bm.faces.new((rings[0][corner], rings[0][next_corner],
                      rings[1][next_corner], rings[1][corner]))
    bm.faces.new(tuple(reversed(rings[0])))
    bm.faces.new(tuple(rings[1]))


def _add_flower(petals, fringe, centre, heading, seed):
    """Build a five-lobed star flower with delicate, visible petal fringes."""
    centre = Vector(centre)
    heading = Vector(heading).normalized()
    for petal_index in range(FLOWER_PETAL_COUNT):
        angle = math.tau * petal_index / FLOWER_PETAL_COUNT + seed * 0.21
        radial = Vector((math.cos(angle), math.sin(angle), 0.0))
        side = Vector((-radial.y, radial.x, 0.0))
        petal_axis = (radial * 0.72 + heading * 0.18 + Vector((0.0, 0.0, -0.24))).normalized()
        petal_centre = centre + petal_axis * 0.006
        _oriented_ellipsoid(
            petals, petal_centre, petal_axis, side,
            0.023, 0.011, 0.0035, segments=6)

        # Two short filaments per lobe keep the bogbean's fringed flower
        # character visible at asset scale without turning it into a fuzzy ball.
        normal = petal_axis.cross(side).normalized()
        for edge in (-1.0, 1.0):
            start = petal_centre + petal_axis * 0.006 + side * edge * 0.003
            end = start + petal_axis * 0.005 + normal * 0.0012
            _fringe_filament(fringe, start, end)


def build(A):
    lib.PLAIN["bogbean_leaf"] = {"BaseColor": [0.19, 0.31, 0.13], "Roughness": 0.9}
    lib.PLAIN["bogbean_leaf_light"] = {"BaseColor": [0.28, 0.40, 0.18], "Roughness": 0.88}
    lib.PLAIN["bogbean_petioles"] = {"BaseColor": [0.23, 0.35, 0.14], "Roughness": 0.92}
    lib.PLAIN["bogbean_flower"] = {"BaseColor": [0.91, 0.84, 0.77], "Roughness": 0.82}
    lib.PLAIN["bogbean_flower_blush"] = {"BaseColor": [0.76, 0.49, 0.53], "Roughness": 0.84}
    lib.PLAIN["bogbean_fringe"] = {"BaseColor": [0.98, 0.91, 0.82], "Roughness": 0.8}
    lib.PLAIN["bogbean_centre"] = {"BaseColor": [0.82, 0.63, 0.28], "Roughness": 0.78}

    leaves = bmesh.new()
    leaf_bases = []
    for cluster_index in range(LEAF_CLUSTERS):
        angle = math.tau * cluster_index / LEAF_CLUSTERS + 0.12
        radial = Vector((math.cos(angle), math.sin(angle), 0.0))
        across = Vector((-radial.y, radial.x, 0.0))
        node = radial * 0.11 + Vector((0.0, 0.0, 0.105 + 0.012 * (cluster_index % 2)))
        petiole_start = radial * 0.035 + Vector((0.0, 0.0, 0.018))
        petiole_mid = petiole_start.lerp(node, 0.58) + Vector((0.0, 0.0, 0.012))
        A.tube(
            "bogbean_petioles", (petiole_start, petiole_mid, node),
            0.0038, segs=6, taper=lambda t: 1.0 - 0.28 * t,
            name="bogbeanPetiole{:02d}".format(cluster_index + 1))

        for leaflet_index in range(LEAFLET_COUNT_PER_CLUSTER):
            leaflet_angle = angle + math.tau * leaflet_index / LEAFLET_COUNT_PER_CLUSTER + 0.06 * (cluster_index % 2)
            leaflet_axis = Vector((math.cos(leaflet_angle), math.sin(leaflet_angle), 0.13)).normalized()
            leaflet_side = Vector((-leaflet_axis.y, leaflet_axis.x, 0.0)).normalized()
            leaflet_centre = node + leaflet_axis * 0.047 + Vector((0.0, 0.0, 0.008 * (leaflet_index % 2)))
            _oriented_ellipsoid(
                leaves, leaflet_centre, leaflet_axis, leaflet_side,
                LEAFLET_LENGTH_M, LEAFLET_WIDTH_M, 0.006, segments=8)
            leaf_bases.append((leaflet_centre, leaflet_axis, LEAFLET_LENGTH_M))

    A._finish(leaves, "bogbeanTrifoliateLeaves", "bogbean_leaf", (0, 0, 0), (0, 0, 0), "box", "Z", 1.0, True, 48)

    # A few paired pale-green midribs distinguish the compound leaves while
    # keeping the broadleaf clump readable as low wet-margin cover.
    for index, (centre, axis, length) in enumerate(leaf_bases):
        start = centre - axis * (length * 0.26) + Vector((0.0, 0.0, 0.003))
        end = centre + axis * (length * 0.27) + Vector((0.0, 0.0, 0.003))
        A.tube(
            "bogbean_leaf_light", (start, start.lerp(end, 0.52), end),
            0.0012, segs=4, taper=lambda t: 1.0 - 0.45 * t,
            name="bogbeanMidrib{:02d}".format(index + 1))

    petals = bmesh.new()
    blush = bmesh.new()
    centres = bmesh.new()
    fringe = bmesh.new()
    for stalk_index in range(FLOWER_STALKS):
        angle = math.tau * stalk_index / FLOWER_STALKS + 0.31
        radial = Vector((math.cos(angle), math.sin(angle), 0.0))
        base = radial * 0.045 + Vector((0.0, 0.0, 0.02))
        height = FLOWER_HEIGHTS_M[stalk_index]
        lean = radial * (0.018 + 0.004 * (stalk_index % 2))
        top = base + Vector((lean.x, lean.y, height))
        mid = base.lerp(top, 0.62) + Vector((0.0, 0.0, 0.012))
        A.tube(
            "bogbean_petioles", (base, mid, top), 0.0032, segs=6,
            taper=lambda t: 1.0 - 0.36 * t,
            name="bogbeanFlowerScape{:02d}".format(stalk_index + 1))

        count = FLOWERS_PER_STALK[stalk_index]
        for flower_index in range(count):
            raceme_span = FLOWER_RACEME_END_T - FLOWER_RACEME_START_T
            t = FLOWER_RACEME_START_T + raceme_span * flower_index / (count - 1)
            flower_centre = base.lerp(top, t) + radial * (0.006 * (flower_index % 2))
            heading = radial * 0.34 + Vector((0.0, 0.0, 0.94))
            flower_mesh = blush if (flower_index + stalk_index) % 4 == 0 else petals
            _add_flower(flower_mesh, fringe, flower_centre, heading, flower_index + stalk_index * 3)
            centre_mesh = bmesh.ops.create_uvsphere(
                centres, u_segments=6, v_segments=4, radius=0.0038, calc_uvs=False)
            for vert in centre_mesh["verts"]:
                vert.co += flower_centre + Vector((0.0, 0.0, 0.002))

    A._finish(petals, "bogbeanWhiteFlowers", "bogbean_flower", (0, 0, 0), (0, 0, 0), "box", "Z", 1.0, True, 48)
    A._finish(blush, "bogbeanBlushedFlowers", "bogbean_flower_blush", (0, 0, 0), (0, 0, 0), "box", "Z", 1.0, True, 48)
    A._finish(centres, "bogbeanFlowerCentres", "bogbean_centre", (0, 0, 0), (0, 0, 0), "box", "Z", 1.0, True, 48)
    A._finish(fringe, "bogbeanPetalFringes", "bogbean_fringe", (0, 0, 0), (0, 0, 0), "box", "Z", 1.0, True, 48)

    # Foliage is decorative and remains collisionless for a safe water-edge trial.
    for obj, _material in A.parts:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(obj.data)
        bm.free()
