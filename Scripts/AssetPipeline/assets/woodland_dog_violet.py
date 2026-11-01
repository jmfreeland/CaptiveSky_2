"""A low, spring woodland-floor colony of common dog violets."""
import math

import bmesh
from mathutils import Vector

import lib


NAME = "WoodlandDogViolet"
PLANT_BASES = ((-0.18, -0.06, 0.006), (0.04, -0.16, 0.006),
               (0.18, 0.04, 0.006), (-0.04, 0.16, 0.006))
FLOWER_COUNT = 5
MODEL_SCALE = 0.70
MAX_HEIGHT_M = 0.15
PROVENANCE = "Collisionless low-poly botanical study, procedurally authored in Blender 5.2; no generated textures or external source mesh."


def _heart_leaf(bm, origin, direction, length, width, phase):
    """Build a shallow, softly cupped heart-shaped leaf with a notched base."""
    direction = Vector(direction).normalized()
    across = Vector((-direction.y, direction.x, 0.0)).normalized()
    origin = Vector(origin)
    outline = ((0.00, 0.00), (0.16, 0.14), (0.34, 0.30),
               (0.54, 0.43), (0.77, 0.38), (0.94, 0.20),
               (1.00, 0.00), (0.94, -0.20), (0.77, -0.38),
               (0.54, -0.43), (0.34, -0.30), (0.16, -0.14))
    center = bm.verts.new(origin + direction * (length * 0.52) + Vector((0.0, 0.0, 0.012)))
    edge = []
    for along, side in outline:
        point = origin + direction * (length * along) + across * (width * side)
        point.z += 0.018 * math.sin(math.pi * along + phase) + width * 0.055 * (1.0 - abs(side) * 2.0)
        edge.append(bm.verts.new(point))
    for index in range(len(edge)):
        bm.faces.new((center, edge[index], edge[(index + 1) % len(edge)]))
    back = [bm.verts.new(vertex.co - Vector((0.0, 0.0, 0.0015))) for vertex in [center] + edge]
    for index in range(len(edge)):
        bm.faces.new((back[0], back[1 + ((index + 1) % len(edge))], back[1 + index]))


def _flower_petal(bm, center, normal, angle, length, width, phase):
    normal = Vector(normal).normalized()
    across = Vector((1.0, 0.0, 0.0))
    across -= normal * across.dot(normal)
    if across.length_squared < 1e-5:
        across = Vector((0.0, 1.0, 0.0))
        across -= normal * across.dot(normal)
    across.normalize()
    side = normal.cross(across).normalized()
    radial = across * math.cos(angle) + side * math.sin(angle)
    lateral = normal.cross(radial).normalized()
    rows = []
    for row in range(5):
        t = row / 4.0
        row_center = Vector(center) + radial * (length * t)
        row_center += normal * (0.004 * math.sin(math.pi * t + phase))
        profile = math.sin(math.pi * (0.08 + 0.84 * t)) ** 0.68
        half_width = width * profile
        verts = []
        for across_factor in (-1.0, 0.0, 1.0):
            point = row_center + lateral * (half_width * across_factor)
            point += normal * (0.0025 * (1.0 - abs(across_factor)) * math.sin(math.pi * t))
            verts.append(bm.verts.new(point))
        rows.append(verts)
    for row in range(4):
        for col in range(2):
            bm.faces.new((rows[row][col], rows[row + 1][col],
                          rows[row + 1][col + 1], rows[row][col + 1]))


def _flower_throat(bm, center, normal):
    normal = Vector(normal).normalized()
    across = Vector((1.0, 0.0, 0.0))
    across -= normal * across.dot(normal)
    if across.length_squared < 1e-5:
        across = Vector((0.0, 1.0, 0.0))
        across -= normal * across.dot(normal)
    across.normalize()
    side = normal.cross(across).normalized()
    center = Vector(center) + normal * 0.0015
    middle = bm.verts.new(center)
    ring = [bm.verts.new(center + 0.006 * (across * math.cos(math.tau * i / 8.0) +
                                             side * math.sin(math.tau * i / 8.0)))
            for i in range(8)]
    for index in range(8):
        bm.faces.new((middle, ring[index], ring[(index + 1) % 8]))


def build(A):
    lib.PLAIN["violet_leaf_dark"] = {"BaseColor": [0.075, 0.19, 0.055], "Roughness": 0.9}
    lib.PLAIN["violet_leaf_light"] = {"BaseColor": [0.16, 0.31, 0.085], "Roughness": 0.88}
    lib.PLAIN["violet_petal"] = {"BaseColor": [0.28, 0.12, 0.48], "Roughness": 0.78}
    lib.PLAIN["violet_throat"] = {"BaseColor": [0.84, 0.70, 0.44], "Roughness": 0.82}

    leaf_bm = bmesh.new()
    petal_bm = bmesh.new()
    throat_bm = bmesh.new()
    flower_locations = []

    for plant_index, base_tuple in enumerate(PLANT_BASES):
        base = Vector(base_tuple)
        phase = plant_index * 1.83 + 0.32
        leaf_count = 4
        for leaf_index in range(leaf_count):
            angle = phase + math.tau * leaf_index / leaf_count
            direction = Vector((math.cos(angle), math.sin(angle), 0.0))
            A.tube("violet_leaf_light", [base, base + direction * 0.045 + Vector((0.0, 0.0, 0.025)),
                    base + direction * 0.10 + Vector((0.0, 0.0, 0.04))],
                   0.0018, segs=3, taper=lambda t: 1.0 - 0.72 * t,
                   name="violetPetiole{:02d}_{:02d}".format(plant_index, leaf_index))
            _heart_leaf(leaf_bm, base + direction * 0.045, direction,
                        0.14 + 0.025 * ((leaf_index + plant_index) % 3),
                        0.09 + 0.012 * ((leaf_index + 2 * plant_index) % 2), phase + leaf_index)
        if plant_index in (0, 1, 2, 3):
            flower_locations.append((base, phase, plant_index))

    for flower_index in range(FLOWER_COUNT):
        base, phase, plant_index = flower_locations[flower_index % len(flower_locations)]
        angle = phase + flower_index * 2.399
        direction = Vector((math.cos(angle), math.sin(angle), 0.0))
        height = 0.105 + 0.018 * ((flower_index * 3) % 4)
        center = base + direction * (0.045 + 0.012 * (flower_index % 3)) + Vector((0.0, 0.0, height))
        A.tube("violet_leaf_light", [base + Vector((0.0, 0.0, 0.012)),
                base + direction * 0.022 + Vector((0.0, 0.0, height * 0.55)), center],
               0.0018, segs=3, taper=lambda t: 1.0 - 0.68 * t,
               name="violetFlowerStem{:02d}".format(flower_index))
        normal = (direction * 0.48 + Vector((0.0, 0.0, 0.88))).normalized()
        for petal_index in range(5):
            petal_angle = math.tau * petal_index / 5.0 + 0.18 * math.sin(phase)
            _flower_petal(petal_bm, center, normal, petal_angle,
                          0.032 if petal_index == 2 else 0.028,
                          0.014 if petal_index == 2 else 0.012,
                          phase + petal_index * 0.4)
        _flower_throat(throat_bm, center, normal)

    for bm, name, material in ((leaf_bm, "violetLeaves", "violet_leaf_dark"),
                               (petal_bm, "violetBlooms", "violet_petal"),
                               (throat_bm, "violetThroats", "violet_throat")):
        if bm.faces:
            bmesh.ops.solidify(bm, geom=bm.faces[:], thickness=0.0015)
            bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
            A._finish(bm, name, material, (0, 0, 0), (0, 0, 0), "box", "Z", 1.0, True, 50)
        else:
            bm.free()

    for obj, _material in A.parts:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        for vertex in bm.verts:
            vertex.co *= MODEL_SCALE
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(obj.data)
        bm.free()
