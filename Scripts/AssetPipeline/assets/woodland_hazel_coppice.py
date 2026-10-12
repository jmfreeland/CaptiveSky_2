"""A dense, open-centred hazel coppice for woodland-edge mid-distance structure."""
import math

import bmesh
from mathutils import Vector

import lib


NAME = "WoodlandHazelCoppice"


def _leaf(bm, base, direction, width_axis, length, half_width, seed):
    """Add one broad, folded, double-sided leaf to a material-batched mesh."""
    direction = Vector(direction).normalized()
    width_axis = Vector(width_axis).normalized()
    normal = direction.cross(width_axis).normalized()
    base = Vector(base)
    rows = (0.0, 0.25, 0.5, 0.75, 1.0)
    silhouette = (0.02, 0.62, 1.0, 0.70, 0.015)
    jitter = 1.0 + 0.045 * math.sin(seed * 2.17)
    front = []
    for row, t in enumerate(rows):
        row_verts = []
        for side in (-1.0, 0.0, 1.0):
            point = base + direction * (length * t)
            point += width_axis * (half_width * silhouette[row] * side * jitter)
            point += normal * (0.025 * math.sin(math.pi * t) * (1.0 - abs(side)))
            point.z += 0.018 * math.sin(math.pi * t) * abs(side)
            row_verts.append(bm.verts.new(point))
        front.append(row_verts)

    back = [[bm.verts.new(v.co - normal * 0.0018) for v in row] for row in front]
    for r in range(len(rows) - 1):
        for c in range(2):
            bm.faces.new((front[r][c], front[r][c + 1], front[r + 1][c + 1], front[r + 1][c]))
            bm.faces.new((back[r][c + 1], back[r][c], back[r + 1][c], back[r + 1][c + 1]))


def build(A):
    lib.PLAIN["coppice_bark"] = {"BaseColor": [0.17, 0.095, 0.045], "Roughness": 0.9}
    lib.PLAIN["coppice_leaf_deep"] = {"BaseColor": [0.10, 0.22, 0.045], "Roughness": 0.86}
    lib.PLAIN["coppice_leaf_mid"] = {"BaseColor": [0.145, 0.285, 0.065], "Roughness": 0.84}
    lib.PLAIN["coppice_leaf_sun"] = {"BaseColor": [0.19, 0.32, 0.085], "Roughness": 0.84}

    palette = ("coppice_leaf_deep", "coppice_leaf_mid", "coppice_leaf_sun")
    leaf_meshes = {material: bmesh.new() for material in palette}
    leaf_index = 0

    def add_spray(anchor, radial, tangent, stem_index, spray_index, scale=1.0):
        nonlocal leaf_index
        up = Vector((0.0, 0.0, 1.0))
        directions = (
            radial + up * 0.48,
            radial * 0.35 + tangent * 0.72 + up * 0.32,
            radial * 0.35 - tangent * 0.72 + up * 0.32,
            radial * 0.18 + tangent * 0.26 + up * 0.82,
            radial * 0.18 - tangent * 0.26 + up * 0.82,
        )
        for leaf_part, raw_direction in enumerate(directions):
            direction = (raw_direction + tangent * (0.06 * ((stem_index + spray_index + leaf_part) % 2))).normalized()
            width_axis = Vector((-direction.y, direction.x, 0.0))
            if width_axis.length < 0.1:
                width_axis = tangent.copy()
            width_axis.normalize()
            material = palette[(stem_index + spray_index + leaf_part) % len(palette)]
            length = scale * (0.29 + 0.025 * ((stem_index + leaf_part) % 3))
            half_width = scale * (0.12 + 0.014 * ((spray_index + leaf_part) % 3))
            _leaf(leaf_meshes[material], anchor, direction, width_axis, length, half_width, leaf_index)
            leaf_index += 1

    # Five irregular stems keep an open centre while their offset crowns merge
    # into a readable mass rather than a symmetrical, clipped hedge.
    stem_count = 5
    for stem in range(stem_count):
        angle = (2.0 * math.pi * stem / stem_count) + 0.16 * math.sin(stem * 2.1)
        radial = Vector((math.cos(angle), math.sin(angle), 0.0))
        tangent = Vector((-radial.y, radial.x, 0.0))
        height = 2.05 + 0.23 * math.sin(stem * 1.71)
        crown_offset = 0.38 + 0.12 * (0.5 + 0.5 * math.sin(stem * 2.37))
        path = []
        for step in range(7):
            t = step / 6.0
            point = radial * (crown_offset * t * t)
            point += tangent * (0.09 * math.sin(math.pi * t + stem * 1.1))
            point.z = 0.07 + height * t
            path.append(point)
        A.tube("coppice_bark", path, 0.048 - 0.016 * stem / stem_count, segs=7,
               taper=lambda t: 1.0 - 0.62 * t, name="coppiceStem_{}".format(stem))

        # Broad sprays along the upper stems make a continuous canopy when
        # seen across the open woodland transition.
        for node, point in enumerate(path[1:]):
            add_spray(point - radial * 0.06, radial, tangent, stem, node, 0.92)

        # Four broad, relaxed side branches fill the lower and outer crown.
        for branch in range(4):
            t = 0.24 + branch * 0.16
            base = path[0].lerp(path[-1], t)
            side = tangent * (1.0 if (stem + branch) % 2 == 0 else -1.0)
            branch_radial = radial * (0.38 + 0.05 * branch) + side * 0.74
            tip = base + branch_radial * (0.56 + 0.035 * branch) + Vector((0.0, 0.0, 0.22 + 0.045 * branch))
            A.tube("coppice_bark", [base, base.lerp(tip, 0.52) + Vector((0.0, 0.0, 0.06)), tip],
                   0.018 - branch * 0.002, segs=6, taper=lambda t: 1.0 - 0.72 * t,
                   name="coppiceBranch_{}_{}".format(stem, branch))
            add_spray(base.lerp(tip, 0.42), branch_radial, side, stem, branch + 4, 0.80)
            add_spray(tip - branch_radial * 0.08, branch_radial, side, stem, branch + 8, 0.76)

    for material, bm in leaf_meshes.items():
        if bm.faces:
            A._finish(bm, material, material, (0, 0, 0), (0, 0, 0), "box", "Z", 1.0, True, 55)
        else:
            bm.free()

    # Keep the base uncollided so the eventual foliage group remains a soft
    # habitat edge; placement and walkability must be decided in Unreal.
    for obj, _material in A.parts:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(obj.data)
        bm.free()
