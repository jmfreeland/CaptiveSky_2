"""A low woodland fern clump with arching, asymmetrical fronds and fresh leaflets."""
import math

import bmesh
from mathutils import Vector

import lib


NAME = "WoodlandFernClump"


def _centerline(angle, length, t):
    # The frond rises first, then relaxes toward its tip instead of forming a cone.
    radius = length * t
    height = 0.075 + 0.83 * t - 0.31 * t * t
    return Vector((math.cos(angle) * radius, math.sin(angle) * radius, height))


def _frond(A, leaf_bm, angle, length, width_scale, index):
    direction = Vector((math.cos(angle), math.sin(angle), 0.0))
    across = Vector((-direction.y, direction.x, 0.0))
    points = [_centerline(angle, length, i / 8) for i in range(9)]
    A.tube("fern_stem", points, 0.008 * width_scale, segs=5,
           taper=lambda t: 1.0 - 0.68 * t, name="fernRachis")

    # Pinnae shrink steadily toward the tip, leaving the silhouette open and
    # feather-like rather than making a solid green ball.
    for row in range(1, 9):
        t = row / 9
        anchor = _centerline(angle, length, t)
        leaflet_length = width_scale * (0.15 * (1.0 - t) + 0.045)
        for side in (-1, 1):
            # Each leaflet sweeps back from the rachis while spreading sideways.
            leaf_direction = (direction * 0.48 - across * side * 0.88).normalized()
            leaf_across = Vector((-leaf_direction.y, leaf_direction.x, 0.0))
            leaf_base = anchor + across * side * 0.006
            widths = (0.006, 0.035, 0.050, 0.032, 0.001)
            rows = []
            for lateral in (-1.0, 0.0, 1.0):
                leaf_row = []
                for step, portion in enumerate((0.0, 0.24, 0.50, 0.78, 1.0)):
                    point = leaf_base + leaf_direction * (leaflet_length * portion)
                    point += leaf_across * (lateral * widths[step] * width_scale)
                    point.z += 0.008 * math.sin(math.pi * portion) * width_scale
                    if lateral == 0.0:
                        point.z += 0.004 * math.sin(math.pi * portion)
                    leaf_row.append(point)
                rows.append(leaf_row)

            leaf_verts = [[leaf_bm.verts.new(point) for point in leaf_row] for leaf_row in rows]
            for r in range(2):
                for c in range(4):
                    leaf_bm.faces.new((
                        leaf_verts[r][c], leaf_verts[r][c + 1],
                        leaf_verts[r + 1][c + 1], leaf_verts[r + 1][c]))


def build(A):
    # These per-asset palette entries extend the pipeline in-memory; the shared
    # toolkit file stays untouched, and the exact colours are preserved in the manifest.
    lib.PLAIN["fern_leaf"] = {"BaseColor": [0.075, 0.19, 0.045], "Roughness": 0.82}
    lib.PLAIN["fern_stem"] = {"BaseColor": [0.16, 0.23, 0.065], "Roughness": 0.88}

    leaf_bm = bmesh.new()
    # Six non-uniform fronds create a broken, natural fan with a readable centre.
    for index, (degrees, length, scale) in enumerate((
            (3, 0.58, 1.00), (61, 0.49, 0.90), (119, 0.63, 1.08),
            (180, 0.52, 0.94), (240, 0.61, 1.04), (300, 0.54, 0.96))):
        _frond(A, leaf_bm, math.radians(degrees), length, scale, index)

    if leaf_bm.faces:
        bmesh.ops.solidify(leaf_bm, geom=leaf_bm.faces[:], thickness=0.0018)
        A._finish(leaf_bm, "fernPinnae", "fern_leaf", (0, 0, 0), (0, 0, 0),
                  "box", "Z", 1.0, True, 55)
    else:
        leaf_bm.free()

    # A small, irregular crown hides the frond junctions without a blocking pot.
    A.rock("fern_leaf", (0.11, 0.10, 0.055), loc=(0.0, 0.0, 0.0275),
           seed=53, sub=1, rough=0.12, flat_bottom=0.5,
           name="fernCrown")

    # Deterministic triangulation avoids exporter tangent warnings on the leaf grids.
    for obj, _material in A.parts:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(obj.data)
        bm.free()
