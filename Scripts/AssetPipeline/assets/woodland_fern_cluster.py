"""A low, layered lady-fern rosette for a shaded temperate woodland floor."""
import math

import bmesh
from mathutils import Vector

import lib


NAME = "WoodlandFernCluster"
FROND_COUNT = 7
PINNAE_PAIRS = 8
PROVENANCE = "Geometry and four plain green material slots procedurally authored in Blender 5.2; no generated textures or external source mesh."


def _leaflet(A, origin, direction, width, length, material, name):
    """Build a softly folded, double-sided leaflet with a small central ridge."""
    direction = Vector(direction).normalized()
    across = Vector((0.0, 0.0, 1.0)).cross(direction)
    if across.length < 0.1:
        across = Vector((1.0, 0.0, 0.0))
    across.normalize()
    normal = direction.cross(across).normalized()
    origin = Vector(origin)

    outline = ((0.0, 0.0), (0.22, -0.32), (0.52, -0.46), (0.78, -0.29),
               (1.0, 0.0), (0.78, 0.29), (0.52, 0.46), (0.22, 0.32))
    bm = bmesh.new()
    center = bm.verts.new(origin + direction * (length * 0.52) + normal * (width * 0.10))
    edge = []
    for along, side in outline:
        point = origin + direction * (length * along) + across * (width * side)
        point += normal * (math.sin(math.pi * along) * width * 0.12)
        edge.append(bm.verts.new(point))
    for index in range(len(edge)):
        bm.faces.new((center, edge[index], edge[(index + 1) % len(edge)]))

    back = [bm.verts.new(vertex.co - normal * 0.001) for vertex in [center] + edge]
    back_center, back_edge = back[0], back[1:]
    for index in range(len(back_edge)):
        bm.faces.new((back_center, back_edge[(index + 1) % len(back_edge)], back_edge[index]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    A._finish(bm, name, material, (0, 0, 0), (0, 0, 0), "box", "Z", 1.0, True, 55)


def build(A):
    lib.PLAIN["fern_rachis"] = {"BaseColor": [0.15, 0.24, 0.055], "Roughness": 0.9}
    lib.PLAIN["fern_deep_green"] = {"BaseColor": [0.075, 0.20, 0.035], "Roughness": 0.88}
    lib.PLAIN["fern_leaf_green"] = {"BaseColor": [0.14, 0.34, 0.055], "Roughness": 0.86}
    lib.PLAIN["fern_young_green"] = {"BaseColor": [0.25, 0.43, 0.09], "Roughness": 0.84}

    foliage_palette = ("fern_deep_green", "fern_leaf_green", "fern_young_green")
    for frond_index in range(FROND_COUNT):
        azimuth = 2.0 * math.pi * frond_index / FROND_COUNT + 0.12 * math.sin(frond_index * 2.1)
        radial = Vector((math.cos(azimuth), math.sin(azimuth), 0.0))
        tangent = Vector((-radial.y, radial.x, 0.0))
        length = 0.37 + 0.075 * (0.5 + 0.5 * math.sin(frond_index * 1.73 + 0.4))
        lift = 0.61 + 0.09 * math.cos(frond_index * 1.31)
        base = radial * (0.012 + 0.012 * (frond_index % 3))

        def spine_at(t):
            radius = 0.012 + length * t
            height = 0.008 + lift * (1.72 * t * (1.0 - t) + 0.25 * t)
            return base + radial * radius + Vector((0.0, 0.0, height))

        A.tube("fern_rachis", [spine_at(0.0), spine_at(0.52), spine_at(1.0)],
               0.0032, segs=5, taper=lambda t: 1.0 - 0.82 * t,
               name="fernRachis{:02d}".format(frond_index))

        # Paired pinnae narrow toward each frond's tip, with modest deterministic
        # variation so the seven overlapping fans do not read as repeated combs.
        for pair_index in range(PINNAE_PAIRS):
            t = 0.18 + 0.085 * pair_index + 0.012 * math.sin(frond_index * 1.4 + pair_index)
            anchor = spine_at(t)
            taper = math.sin(math.pi * (0.12 + 0.80 * t))
            leaflet_length = (0.050 + 0.075 * taper) * (0.91 + 0.08 * math.cos(frond_index + pair_index * 0.7))
            leaflet_width = leaflet_length * (0.25 + 0.025 * ((frond_index + pair_index) % 3))
            for side in (-1.0, 1.0):
                direction = (radial * 0.28 + tangent * side * 0.86 + Vector((0.0, 0.0, -0.30)))
                direction.normalize()
                material = foliage_palette[(frond_index + pair_index + int(side > 0)) % len(foliage_palette)]
                _leaflet(A, anchor, direction, leaflet_width, leaflet_length, material,
                         "fernPinna{:02d}_{:02d}_{}".format(frond_index, pair_index, int(side > 0)))

    # Triangulate the curved rachises so exported tangents are deterministic.
    for obj, _material in A.parts:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(obj.data)
        bm.free()
