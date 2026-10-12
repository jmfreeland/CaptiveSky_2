"""A small silver-birch coppice to add pale trunks and a broadleaf canopy at the woodland edge."""
import math

import bmesh
from mathutils import Vector

import lib


NAME = "BirchCoppice"


def _leaf(A, origin, direction, width, length, material, name):
    """Build a softly folded, double-sided ovate leaf with a central ridge."""
    direction = Vector(direction).normalized()
    across = Vector((-direction.y, direction.x, 0.0))
    if across.length < 0.1:
        across = Vector((1.0, 0.0, 0.0))
    across.normalize()
    origin = Vector(origin)
    normal = direction.cross(across).normalized()

    # A fuller outline than a simple diamond keeps the small canopy legible at
    # mid-distance without turning the leaf into a flat triangular billboard.
    outline = ((0.0, 0.0), (0.22, -0.34), (0.53, -0.48), (0.76, -0.30),
               (1.0, 0.0), (0.76, 0.30), (0.53, 0.48), (0.22, 0.34))
    bm = bmesh.new()
    center = bm.verts.new(origin + direction * (length * 0.52) + normal * (width * 0.10))
    edge = []
    for along, side in outline:
        point = origin + direction * (length * along) + across * (width * side)
        point += normal * (math.sin(math.pi * along) * width * 0.10)
        point.z += abs(side) * width * 0.025
        edge.append(bm.verts.new(point))
    for index in range(len(edge)):
        bm.faces.new((center, edge[index], edge[(index + 1) % len(edge)]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    back = [bm.verts.new(vertex.co - normal * 0.0015) for vertex in [center] + edge]
    back_center, back_edge = back[0], back[1:]
    for index in range(len(back_edge)):
        bm.faces.new((back_center, back_edge[(index + 1) % len(back_edge)], back_edge[index]))
    A._finish(bm, name, material, (0, 0, 0), (0, 0, 0), "box", "Z", 1.0, True, 55)


def build(A):
    lib.PLAIN["birch_bark"] = {"BaseColor": [0.63, 0.60, 0.51], "Roughness": 0.91}
    lib.PLAIN["birch_bark_mark"] = {"BaseColor": [0.16, 0.17, 0.15], "Roughness": 0.94}
    lib.PLAIN["birch_leaf_shadow"] = {"BaseColor": [0.15, 0.31, 0.075], "Roughness": 0.86}
    lib.PLAIN["birch_leaf_green"] = {"BaseColor": [0.26, 0.43, 0.11], "Roughness": 0.84}
    lib.PLAIN["birch_leaf_sun"] = {"BaseColor": [0.39, 0.54, 0.17], "Roughness": 0.82}

    # Five close, slightly leaning stems form one readable coppice rather than
    # a lonely sapling or a wall of vegetation.  The variation opens sightlines
    # through the lower trunk while the upper branches overlap into a crown.
    stems = []
    for index in range(5):
        angle = 2.0 * math.pi * index / 5.0 + 0.13 * math.sin(index * 2.1)
        radial = Vector((math.cos(angle), math.sin(angle), 0.0))
        tangent = Vector((-radial.y, radial.x, 0.0))
        radius = (0.10, 0.29, 0.24, 0.33, 0.13)[index]
        height = 2.45 + 0.31 * math.sin(index * 1.63) + (0.18 if index == 2 else 0.0)
        lean = (0.10 + 0.035 * (index % 3)) * (-1.0 if index % 2 else 1.0)
        path = []
        for step in range(7):
            t = step / 6.0
            point = radial * (radius + lean * t * t) + tangent * (0.045 * math.sin(math.pi * t + index))
            point.z = 0.0075 + height * t
            path.append(point)
        stems.append((radial, tangent, height, path))
        A.tube("birch_bark", path, 0.066 - 0.007 * index, segs=9,
               taper=lambda t: 1.0 - 0.72 * t, name="birchTrunk_{}".format(index))

        # Sparse charcoal lenticel marks break up the pale bark without adding
        # heavy rings or a second texture set.
        for mark in range(3):
            t = 0.35 + 0.18 * mark
            center = path[0].lerp(path[-1], t)
            across = tangent * (0.08 + 0.025 * (mark % 2))
            A.tube("birch_bark_mark", [center - across, center, center + across],
                   0.0055, segs=5, taper=lambda v: 0.85 - 0.55 * v,
                   name="birchLenticel_{}_{}".format(index, mark))

        # Branch fans are concentrated in the upper two thirds, with a few
        # descending side boughs to make a rounded, layered canopy.
        for branch in range(11):
            t = max(0.28, min(0.91,
                0.28 + 0.054 * branch + 0.035 * math.sin(index * 1.4 + branch * 2.4)))
            base = path[0].lerp(path[-1], t)
            branch_angle = angle + math.radians(-54 + 10.5 * branch + 11 * math.sin(index + branch))
            out = Vector((math.cos(branch_angle), math.sin(branch_angle), 0.0))
            reach = 0.38 + 0.08 * math.sin(index * 1.7 + branch)
            rise = 0.26 + 0.10 * (branch % 3) + 0.07 * math.sin(index * 1.9 + branch)
            mid = base + out * (reach * 0.52) + Vector((0, 0, rise * 0.52))
            tip = base + out * reach + Vector((0, 0, rise))
            A.tube("birch_bark", [base, mid, tip], 0.021, segs=6,
                   taper=lambda v: 1.0 - 0.82 * v,
                   name="birchBough_{}_{}".format(index, branch))

            # Leaf sprays at both the branch shoulder and tip fill the gaps
            # between trunks.  Direction and color vary deterministically.
            palette = ("birch_leaf_shadow", "birch_leaf_green", "birch_leaf_sun")
            for cluster, anchor in enumerate((base.lerp(tip, 0.22), base.lerp(tip, 0.43),
                                              base.lerp(tip, 0.64), base.lerp(tip, 0.85))):
                for leaf_index in range(7):
                    side = -1.0 if leaf_index % 2 else 1.0
                    angle_offset = math.radians(-58 + leaf_index * 19)
                    leaf_dir = (out * math.cos(angle_offset) + tangent * (side * math.sin(angle_offset))
                                + Vector((0, 0, 0.30 + 0.10 * (leaf_index % 3))))
                    leaf_dir.normalize()
                    origin = (anchor - leaf_dir * 0.10 + out * (0.055 * math.cos(angle_offset))
                              + tangent * (0.065 * math.sin(angle_offset)))
                    material = palette[(index + branch + leaf_index + cluster) % len(palette)]
                    _leaf(A, origin, leaf_dir, 0.155 + 0.014 * (leaf_index % 3),
                          0.205 + 0.018 * ((index + branch) % 3), material,
                          "birchLeaf_{}_{}_{}_{}".format(index, branch, cluster, leaf_index))

    # A small ground-reaching sucker and a pair of short catkin-like clusters
    # give the base and crown distinctive details at close viewing distance.
    for sucker, degrees in enumerate((42, 210, 298)):
        direction = Vector((math.cos(math.radians(degrees)), math.sin(math.radians(degrees)), 0.0))
        start = direction * 0.20 + Vector((0, 0, 0.0075))
        end = direction * (0.40 + 0.035 * sucker) + Vector((0, 0, 0.63 + 0.12 * sucker))
        A.tube("birch_bark", [start, start.lerp(end, 0.52), end], 0.027, segs=6,
               taper=lambda t: 1.0 - 0.78 * t, name="birchSucker_{}".format(sucker))
        for leaf_index in range(3):
            leaf_dir = (direction + Vector((0, 0, 0.55 + 0.10 * leaf_index))).normalized()
            _leaf(A, end - leaf_dir * 0.08, leaf_dir, 0.085, 0.135,
                  "birch_leaf_green", "birchSuckerLeaf_{}_{}".format(sucker, leaf_index))

    # Stable triangulation makes FBX tangent generation deterministic.
    for obj, _material in A.parts:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(obj.data)
        bm.free()
