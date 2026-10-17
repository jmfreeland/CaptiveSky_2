"""A low, wind-brushed sea-thrift tuft for the salt-sprayed Tideglass shoreline."""
import math

import bmesh
from mathutils import Vector

import lib


NAME = "TideglassSeaThrift"
PROVENANCE = "Geometry procedurally authored in Blender 5.2; four flat-color material slots; no generated textures."
STEM_BASES = (
    (-0.16, -0.07, 0.006), (-0.08, 0.13, 0.006), (0.04, -0.15, 0.006),
    (0.13, 0.09, 0.006), (-0.02, 0.01, 0.006), (0.18, -0.04, 0.006),
)
FLOWER_HEADS = (
    (-0.14, -0.07, 0.305), (-0.08, 0.13, 0.372), (0.04, -0.15, 0.334),
    (0.13, 0.09, 0.398), (-0.02, 0.01, 0.292), (0.18, -0.04, 0.356),
)
ROSETTE_LEAVES_PER_STEM = 4
FLORETS_PER_HEAD = 15
ROSETTE_LEAF_LENGTH_RANGE_METERS = (0.13, 0.18)
ROSETTE_LEAF_WIDTH_RANGE_METERS = (0.007, 0.009)
LEAF_COLOR_LINEAR = (0.22, 0.36, 0.17)


def _leaf(leaf_bm, origin, angle, length, width, phase):
    direction = Vector((math.cos(angle), math.sin(angle), 0.0))
    across = Vector((-direction.y, direction.x, 0.0))
    rows = []
    for row in range(7):
        t = row / 6
        centre = origin + direction * (length * t)
        centre += across * (0.018 * math.sin(math.pi * t + phase))
        # A short, raised arch keeps the rosette legible as a cushioned tuft
        # instead of a flat star of crossing blades.
        centre.z += 0.025 * t + 0.035 * math.sin(math.pi * t)
        centre.z += 0.008 + 0.006 * (1.0 + math.sin(math.pi * t + phase))
        profile = math.sin(math.pi * t) ** 0.72
        half_width = width * profile * (0.58 + 0.42 * (1.0 - t))
        row_verts = []
        for side in (-1.0, 0.0, 1.0):
            point = centre + across * (side * half_width)
            point.z += 0.004 * (1.0 - side * side) * math.sin(math.pi * t)
            row_verts.append(leaf_bm.verts.new(point))
        rows.append(row_verts)
    for row in range(6):
        for column in range(2):
            leaf_bm.faces.new((rows[row][column], rows[row + 1][column],
                                rows[row + 1][column + 1], rows[row][column + 1]))


def _flower_direction(index, count):
    # A stable Fibonacci-like shell keeps the head rounded without random noise.
    z = 1.0 - 2.0 * (index + 0.5) / count
    angle = index * 2.399963229728653
    radius = math.sqrt(max(0.0, 1.0 - z * z))
    return Vector((math.cos(angle) * radius, math.sin(angle) * radius, z))


def _floret(petal_bm, centre_bm, position, outward, phase):
    normal = outward.normalized()
    across = Vector((0.0, 0.0, 1.0)).cross(normal)
    if across.length_squared < 1e-6:
        across = Vector((1.0, 0.0, 0.0))
    across.normalize()
    side = normal.cross(across).normalized()
    for petal_index in range(5):
        angle = phase + math.tau * petal_index / 5.0
        radial = across * math.cos(angle) + side * math.sin(angle)
        tangent = -across * math.sin(angle) + side * math.cos(angle)
        root = position + normal * 0.001
        shoulder = position + normal * 0.005 + radial * 0.004
        tip = position + normal * 0.008 + radial * 0.010
        left = shoulder - tangent * 0.0035
        right = shoulder + tangent * 0.0035
        verts = [petal_bm.verts.new(point) for point in (root, left, tip, right)]
        petal_bm.faces.new((verts[0], verts[1], verts[2]))
        petal_bm.faces.new((verts[0], verts[2], verts[3]))

    # Tiny warm centres remain visible as a close-up detail, not as bright dots at distance.
    disk_centre = position + normal * 0.0085
    ring = [centre_bm.verts.new(
        disk_centre + across * (0.0028 * math.cos(math.tau * edge / 7.0))
        + side * (0.0028 * math.sin(math.tau * edge / 7.0)))
        for edge in range(7)]
    hub = centre_bm.verts.new(disk_centre + normal * 0.001)
    for edge in range(7):
        centre_bm.faces.new((hub, ring[edge], ring[(edge + 1) % 7]))


def build(A):
    lib.PLAIN["sea_thrift_leaf"] = {"BaseColor": list(LEAF_COLOR_LINEAR), "Roughness": 0.91}
    lib.PLAIN["sea_thrift_stem"] = {"BaseColor": [0.22, 0.31, 0.15], "Roughness": 0.88}
    lib.PLAIN["sea_thrift_petal"] = {"BaseColor": [0.76, 0.34, 0.56], "Roughness": 0.80}
    lib.PLAIN["sea_thrift_centre"] = {"BaseColor": [0.84, 0.64, 0.31], "Roughness": 0.84}

    leaf_bm = bmesh.new()
    petal_bm = bmesh.new()
    centre_bm = bmesh.new()

    for plant_index, (base_tuple, head_tuple) in enumerate(zip(STEM_BASES, FLOWER_HEADS)):
        base = Vector(base_tuple)
        head = Vector(head_tuple)
        lean = Vector((0.025 * math.sin(plant_index * 1.7),
                       0.022 * math.cos(plant_index * 1.3), 0.0))
        top = head + lean
        path = [base.lerp(top, step / 6.0)
                + Vector((0.0, 0.0, 0.007 * math.sin(math.pi * step / 6.0)))
                for step in range(7)]
        A.tube("sea_thrift_stem", path, 0.0036, segs=5,
               taper=lambda t: 1.0 - 0.22 * t,
               name="seaThriftScape{:02d}".format(plant_index + 1))

        # Narrow, arching leaves make a loose coastal cushion around the flower scapes.
        for leaf_index in range(ROSETTE_LEAVES_PER_STEM):
            angle = plant_index * 1.97 + math.tau * leaf_index / ROSETTE_LEAVES_PER_STEM
            origin = base + Vector((0.0, 0.0, 0.006 + 0.001 * (leaf_index % 2)))
            length_t = ((plant_index + leaf_index) % 4) / 3.0
            width_t = ((plant_index + 2 * leaf_index) % 3) / 2.0
            length = ROSETTE_LEAF_LENGTH_RANGE_METERS[0] + (
                ROSETTE_LEAF_LENGTH_RANGE_METERS[1] - ROSETTE_LEAF_LENGTH_RANGE_METERS[0]) * length_t
            width = ROSETTE_LEAF_WIDTH_RANGE_METERS[0] + (
                ROSETTE_LEAF_WIDTH_RANGE_METERS[1] - ROSETTE_LEAF_WIDTH_RANGE_METERS[0]) * width_t
            _leaf(leaf_bm, origin, angle, length, width, angle * 0.37)

        # Each round head is built from individual tiny five-petal florets.
        for floret_index in range(FLORETS_PER_HEAD):
            outward = _flower_direction(floret_index, FLORETS_PER_HEAD)
            position = top + outward * 0.027
            _floret(petal_bm, centre_bm, position, outward,
                    plant_index * 0.41 + floret_index * 0.19)

    if leaf_bm.faces:
        bmesh.ops.solidify(leaf_bm, geom=leaf_bm.faces[:], thickness=0.0014)
        A._finish(leaf_bm, "seaThriftLeaves", "sea_thrift_leaf", (0, 0, 0), (0, 0, 0),
                  "box", "Z", 1.0, True, 48)
    else:
        leaf_bm.free()

    if petal_bm.faces:
        A._finish(petal_bm, "seaThriftFlorets", "sea_thrift_petal", (0, 0, 0), (0, 0, 0),
                  "box", "Z", 1.0, True, 48)
    else:
        petal_bm.free()

    if centre_bm.faces:
        A._finish(centre_bm, "seaThriftFloretCentres", "sea_thrift_centre", (0, 0, 0), (0, 0, 0),
                  "box", "Z", 1.0, True, 48)
    else:
        centre_bm.free()

    # Static ground-cover instances must not snag the player or affect navigation.
    for obj, _material in A.parts:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(obj.data)
        bm.free()
