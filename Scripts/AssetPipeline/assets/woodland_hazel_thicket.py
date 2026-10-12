"""A multi-stem hazel thicket for readable woodland-edge structure."""
import math

import bmesh
from mathutils import Vector

import lib


NAME = "WoodlandHazelThicket"


def _leaf(A, origin, direction, width, length, material, name):
    """Make one double-sided, gently folded leaf with a central ridge."""
    direction = Vector(direction).normalized()
    across = Vector((-direction.y, direction.x, 0.0))
    if across.length < 0.1:
        across = Vector((1.0, 0.0, 0.0))
    across.normalize()
    origin = Vector(origin)
    normal = direction.cross(across).normalized()
    bm = bmesh.new()
    def point(t, side):
        p = origin + direction * (length * t) + across * (width * side)
        p += normal * (math.sin(math.pi * t) * width * 0.10)
        p.z += abs(side) * width * 0.035
        return p

    front = [
        bm.verts.new(point(0.0, 0.0)),
        bm.verts.new(point(0.48, -0.50)),
        bm.verts.new(point(0.50, 0.0)),
        bm.verts.new(point(0.48, 0.50)),
        bm.verts.new(point(1.0, 0.0)),
    ]
    for face in ((front[0], front[1], front[2]), (front[1], front[4], front[2]),
                 (front[2], front[4], front[3]), (front[0], front[2], front[3])):
        bm.faces.new(face)
    back = [bm.verts.new(v.co - normal * 0.0015) for v in front]
    for face in ((back[2], back[1], back[0]), (back[2], back[4], back[1]),
                 (back[3], back[4], back[2]), (back[3], back[2], back[0])):
        bm.faces.new(face)
    A._finish(bm, name, material, (0, 0, 0), (0, 0, 0), "box", "Z", 1.0, True, 55)


def build(A):
    lib.PLAIN["hazel_bark"] = {"BaseColor": [0.19, 0.105, 0.052], "Roughness": 0.91}
    lib.PLAIN["hazel_leaf_deep"] = {"BaseColor": [0.065, 0.145, 0.035], "Roughness": 0.86}
    lib.PLAIN["hazel_leaf_mid"] = {"BaseColor": [0.105, 0.205, 0.052], "Roughness": 0.84}
    lib.PLAIN["hazel_leaf_light"] = {"BaseColor": [0.17, 0.255, 0.075], "Roughness": 0.83}

    # Several independent, outward-leaning stems create a loose but leafy mass,
    # leaving gaps so this reads as a thicket rather than a clipped hedge.
    stem_count = 9
    for stem in range(stem_count):
        angle = 2.0 * math.pi * stem / stem_count + 0.19 * math.sin(stem * 1.7)
        radial = Vector((math.cos(angle), math.sin(angle), 0.0))
        tangent = Vector((-radial.y, radial.x, 0.0))
        height = 1.92 + 0.27 * math.sin(stem * 2.3) + (0.12 if stem in (1, 5) else 0.0)
        spread = 0.57 + 0.20 * (0.5 + 0.5 * math.sin(stem * 1.9))
        path = []
        for step in range(6):
            t = step / 5.0
            point = radial * (spread * t * t) + tangent * (0.075 * math.sin(math.pi * t + stem))
            point.z = 0.06 + height * t
            path.append(point)
        A.tube("hazel_bark", path, 0.044 - 0.019 * (stem / stem_count), segs=7,
               taper=lambda t: 1.0 - 0.64 * t, name="hazelStem_{}".format(stem))

        # Alternate leaves along the primary stems as well as on side shoots;
        # this closes the bare central and upper gaps without using leaf blobs.
        stem_leaf_dirs = (radial + Vector((0, 0, 0.24)),
                          tangent * 0.72 + radial * 0.28 + Vector((0, 0, 0.30)),
                          -tangent * 0.72 + radial * 0.28 + Vector((0, 0, 0.30)))
        for node, point in enumerate(path[2:]):
            for leaf_index, raw_direction in enumerate(stem_leaf_dirs):
                direction = (raw_direction + tangent * (0.08 * ((node + leaf_index) % 2))).normalized()
                _leaf(A, point - direction * 0.075, direction, 0.126, 0.215,
                      ("hazel_leaf_deep", "hazel_leaf_mid", "hazel_leaf_light")[(stem + node + leaf_index) % 3],
                      "hazelMainLeaf_{}_{}_{}".format(stem, node, leaf_index))

        # Short side shoots break the silhouette and carry smaller leaf sprays.
        for shoot in range(5):
            t = 0.34 + shoot * 0.12
            base = path[0].lerp(path[-1], t)
            side = tangent * (1.0 if shoot % 2 == 0 else -1.0)
            tip = base + radial * (0.13 + 0.018 * shoot) + side * (0.14 + 0.02 * shoot)
            tip.z += 0.11 + 0.02 * shoot
            A.tube("hazel_bark", [base, base + side * 0.07 + Vector((0, 0, 0.055)), tip],
                   0.014, segs=5, taper=lambda v: 1.0 - 0.8 * v,
                   name="hazelShoot_{}_{}".format(stem, shoot))

            leaf_directions = (
                radial + Vector((0, 0, 0.18)), side * 0.75 + radial * 0.30 + Vector((0, 0, 0.27)),
                radial * 0.48 - side * 0.72 + Vector((0, 0, 0.20)),
                -radial * 0.42 + side * 0.48 + Vector((0, 0, 0.30)),
            )
            palette = ("hazel_leaf_deep", "hazel_leaf_mid", "hazel_leaf_light")
            for cluster, anchor in enumerate((base.lerp(tip, 0.53), tip)):
                for leaf_index, raw_direction in enumerate(leaf_directions):
                    spread_dir = (raw_direction + side * (0.10 * (leaf_index % 2))).normalized()
                    leaf_origin = anchor - spread_dir * 0.10 + side * (0.015 * (leaf_index % 2))
                    material = palette[(stem + shoot + leaf_index + cluster) % len(palette)]
                    _leaf(A, leaf_origin, spread_dir, 0.132 + 0.012 * ((stem + leaf_index) % 3),
                          0.235 + 0.018 * ((stem + shoot) % 3), material,
                          "hazelLeaf_{}_{}_{}_{}".format(stem, shoot, cluster, leaf_index))

    # Keep small basal suckers and their first leaves close to the grounded stems.
    for sucker, degrees in enumerate((32, 154, 267)):
        angle = math.radians(degrees)
        direction = Vector((math.cos(angle), math.sin(angle), 0.0))
        start = Vector((0.0, 0.0, 0.03))
        end = direction * (0.34 + 0.04 * (sucker % 2)) + Vector((0, 0, 0.72 + 0.09 * sucker))
        A.tube("hazel_bark", [start, end * 0.55, end], 0.022, segs=6,
               taper=lambda t: 1.0 - 0.7 * t, name="hazelSucker_{}".format(sucker))
        for leaf_index in range(2):
            leaf_dir = (direction + Vector((0.35 if leaf_index else -0.3, 0.0, 0.7))).normalized()
            _leaf(A, end - leaf_dir * 0.08, leaf_dir, 0.078, 0.17,
                  "hazel_leaf_mid", "hazelSuckerLeaf_{}_{}".format(sucker, leaf_index))

    # One deterministic triangulation pass keeps the FBX tangent generation stable.
    for obj, _material in A.parts:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(obj.data)
        bm.free()
