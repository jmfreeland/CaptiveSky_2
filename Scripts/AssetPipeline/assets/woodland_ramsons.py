"""A loose spring patch of ramsons with basal leaves and starry white umbels."""

import math

import bmesh
from mathutils import Vector

import lib


NAME = "WoodlandRamsons"
ROSETTES = (
    (-0.34, -0.18, 0.20, 0.92),
    (0.02, -0.34, 1.46, 1.00),
    (0.36, -0.22, 2.52, 0.82),
    (-0.42, 0.21, 0.78, 0.80),
    (-0.04, 0.13, 2.08, 1.00),
    (0.34, 0.19, 0.04, 0.88),
    (-0.11, 0.41, 1.26, 0.78),
)
FLOWERING_ROSETTES = (0, 3, 6)
FLORETS_PER_UMBEL = 16
PROVENANCE = (
    "Procedurally authored in Blender 5.2 from a botanical form reference; "
    "collisionless, with no external mesh or generated texture."
)


def _leaflet(bm, origin, angle, length, width, phase):
    direction = Vector((math.cos(angle), math.sin(angle), 0.0))
    across = Vector((-direction.y, direction.x, 0.0))
    rows = []
    for row in range(9):
        t = row / 8
        half_width = width * (math.sin(math.pi * t) ** 0.72)
        centre = origin + direction * (length * t)
        centre += across * (0.025 * length * math.sin(math.pi * t + phase))
        centre.z += 0.075 * math.sin(math.pi * t) - 0.035 * t
        left = centre - across * half_width
        right = centre + across * half_width
        fold = width * 0.12 * math.sin(math.pi * t)
        left.z -= fold
        right.z -= fold
        rows.append((bm.verts.new(left), bm.verts.new(centre), bm.verts.new(right)))

    for row in range(8):
        for side in range(2):
            bm.faces.new((rows[row][side], rows[row + 1][side],
                          rows[row + 1][side + 1], rows[row][side + 1]))


def _star_floret(bm, centre, normal, radius, phase):
    normal = Vector(normal).normalized()
    tangent = normal.cross(Vector((0.0, 0.0, 1.0)))
    if tangent.length < 1e-4:
        tangent = normal.cross(Vector((1.0, 0.0, 0.0)))
    tangent.normalize()
    bitangent = normal.cross(tangent).normalized()
    centre = Vector(centre)
    inner_radius = radius * 0.14

    for petal in range(6):
        angle = phase + math.tau * petal / 6
        radial = tangent * math.cos(angle) + bitangent * math.sin(angle)
        side = -tangent * math.sin(angle) + bitangent * math.cos(angle)
        base = centre + radial * inner_radius
        left = base - side * (radius * 0.14)
        right = base + side * (radius * 0.14)
        tip = centre + radial * radius
        bm.faces.new((bm.verts.new(left), bm.verts.new(tip), bm.verts.new(right)))


def _pollen_dot(bm, centre, normal):
    normal = Vector(normal).normalized()
    tangent = normal.cross(Vector((0.0, 0.0, 1.0)))
    if tangent.length < 1e-4:
        tangent = normal.cross(Vector((1.0, 0.0, 0.0)))
    tangent.normalize()
    bitangent = normal.cross(tangent).normalized()
    centre = Vector(centre)
    top = bm.verts.new(centre + normal * 0.0032)
    bottom = bm.verts.new(centre - normal * 0.0024)
    ring = []
    for corner in range(4):
        angle = math.tau * corner / 4
        ring.append(bm.verts.new(centre + tangent * (0.0022 * math.cos(angle))
                                 + bitangent * (0.0022 * math.sin(angle))))
    for corner in range(4):
        following = (corner + 1) % 4
        bm.faces.new((top, ring[corner], ring[following]))
        bm.faces.new((bottom, ring[following], ring[corner]))


def build(A):
    lib.PLAIN["ramson_leaf_deep"] = {"BaseColor": [0.10, 0.24, 0.055], "Roughness": 0.88}
    lib.PLAIN["ramson_leaf_fresh"] = {"BaseColor": [0.18, 0.36, 0.085], "Roughness": 0.84}
    lib.PLAIN["ramson_stem"] = {"BaseColor": [0.13, 0.25, 0.045], "Roughness": 0.90}
    lib.PLAIN["ramson_flower"] = {"BaseColor": [0.91, 0.89, 0.75], "Roughness": 0.82}
    lib.PLAIN["ramson_pollen"] = {"BaseColor": [0.91, 0.66, 0.18], "Roughness": 0.74}

    leaves = bmesh.new()
    fresh_leaves = bmesh.new()
    flowering_bases = []
    for plant_index, (x, y, heading, scale) in enumerate(ROSETTES):
        origin = Vector((x, y, 0.008 + 0.002 * (plant_index % 3)))
        for leaf_index in range(2):
            angle = heading + math.pi * leaf_index + 0.16 * math.sin(plant_index * 1.7 + leaf_index)
            length = (0.245 + 0.028 * ((plant_index + leaf_index) % 3)) * scale
            width = (0.071 + 0.008 * ((plant_index + 2 * leaf_index) % 3)) * scale
            target = fresh_leaves if (plant_index + leaf_index) % 3 == 0 else leaves
            _leaflet(target, origin, angle, length, width, plant_index * 0.7 + leaf_index)

        if plant_index in FLOWERING_ROSETTES:
            flowering_bases.append((origin, heading, plant_index))

    bmesh.ops.solidify(leaves, geom=leaves.faces[:], thickness=0.0012)
    A._finish(leaves, "ramsonLeaves", "ramson_leaf_deep", (0, 0, 0), (0, 0, 0),
              "box", "Z", 1.0, True, 52)
    bmesh.ops.solidify(fresh_leaves, geom=fresh_leaves.faces[:], thickness=0.0012)
    A._finish(fresh_leaves, "ramsonFreshLeaves", "ramson_leaf_fresh", (0, 0, 0), (0, 0, 0),
              "box", "Z", 1.0, True, 52)

    flowers = bmesh.new()
    pollen = bmesh.new()
    for flower_index, (origin, heading, plant_index) in enumerate(flowering_bases):
        stem_height = 0.235 + 0.018 * (flower_index % 2)
        lean = Vector((0.026 * math.cos(heading + 0.6),
                       0.024 * math.sin(heading + 0.6), 0.0))
        stem_path = [origin + Vector((0, 0, 0.012)),
                     origin + lean * 0.38 + Vector((0, 0, stem_height * 0.52)),
                     origin + lean + Vector((0, 0, stem_height))]
        A.tube("ramson_stem", stem_path, 0.0033, segs=5,
               taper=lambda t: 1.0 - 0.32 * t,
               name="ramsonScape{:02d}".format(flower_index))

        head = stem_path[-1]
        for floret_index in range(FLORETS_PER_UMBEL):
            azimuth = math.tau * floret_index / FLORETS_PER_UMBEL + flower_index * 0.37
            radial = Vector((math.cos(azimuth), math.sin(azimuth), 0.0))
            dome = 0.018 * (0.5 + 0.5 * math.sin(floret_index * 2.1 + flower_index))
            centre = head + radial * (0.032 + dome * 0.28) + Vector((0, 0, dome))
            normal = (radial * 0.62 + Vector((0, 0, 0.78))).normalized()
            radius = 0.017 + 0.002 * ((floret_index + flower_index) % 3)
            _star_floret(flowers, centre, normal, radius, azimuth * 0.4)
            _pollen_dot(pollen, centre + normal * 0.0006, normal)

    bmesh.ops.solidify(flowers, geom=flowers.faces[:], thickness=0.0008)
    A._finish(flowers, "ramsonFlowers", "ramson_flower", (0, 0, 0), (0, 0, 0),
              "box", "Z", 1.0, True, 45)

    # The small flower centers share a single grouped pollen mesh.
    A._finish(pollen, "ramsonPollen", "ramson_pollen", (0, 0, 0), (0, 0, 0),
              "box", "Z", 1.0, True, 45)

    for obj, _material in A.parts:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(obj.data)
        bm.free()
