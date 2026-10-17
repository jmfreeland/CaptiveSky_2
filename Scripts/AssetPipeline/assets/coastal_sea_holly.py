"""A blue-grey, spiny sea-holly rosette for a dry, open foredune."""
import math

import bmesh
from mathutils import Vector

import lib


NAME = "CoastalSeaHolly"
PROVENANCE = "Deterministic Eryngium maritimum-inspired geometry procedurally authored in Blender 5.2; three flat-color material slots; no generated textures."
ROSETTE_LEAVES = 5
LEAF_LENGTHS = (0.27, 0.24, 0.29, 0.26, 0.25)
FLOWER_HEADS = (
    (-0.045, -0.025, 0.43), (0.065, 0.045, 0.37),
    (-0.105, 0.07, 0.34), (0.12, -0.09, 0.32),
)
BRACTS_PER_HEAD = 20
LEAF_STATIONS = 11
SPINY_STATIONS = (2, 5, 8)


def _leaf(bm, origin, angle, length, phase):
    """A low, stiff, lobed blade with three pairs of small terminal spines."""
    radial = Vector((math.cos(angle), math.sin(angle), 0.0))
    across = Vector((-radial.y, radial.x, 0.0))
    axis = (radial * 0.93 + Vector((0.0, 0.0, 0.26))).normalized()
    widths = (0.0, 0.18, 0.46, 0.40, 0.50, 0.40,
              0.46, 0.34, 0.38, 0.18, 0.0)
    rows = []
    for station, width_ratio in enumerate(widths):
        t = station / (LEAF_STATIONS - 1)
        centre = origin + axis * (length * t)
        centre.z += 0.004 * math.sin(math.pi * t + phase)
        width = length * 0.24 * width_ratio
        row = []
        for side in (-1.0, 0.0, 1.0):
            point = centre + across * (side * width)
            point.z += (0.003 * math.sin(math.pi * t) if side == 0.0 else 0.0)
            row.append(bm.verts.new(point))
        rows.append(row)

    for station in range(LEAF_STATIONS - 1):
        for column in range(2):
            bm.faces.new((rows[station][column], rows[station + 1][column],
                          rows[station + 1][column + 1], rows[station][column + 1]))

    # Paired sharp lobes make the silhouette read as sea holly rather than grass.
    for station in SPINY_STATIONS:
        t = station / (LEAF_STATIONS - 1)
        centre = origin + axis * (length * t)
        width = length * 0.24 * widths[station]
        for side in (-1.0, 1.0):
            edge = centre + across * (side * width)
            tip = edge + across * (side * 0.024)
            tip += axis * (length * 0.035)
            next_t = (station + 1) / (LEAF_STATIONS - 1)
            next_centre = origin + axis * (length * next_t)
            next_width = length * 0.24 * widths[station + 1]
            next_edge = next_centre + across * (side * next_width)
            bm.faces.new((bm.verts.new(edge), bm.verts.new(tip), bm.verts.new(next_edge)))


def _bract_head(bm, centre, phase):
    """An open spherical spray of tapered blue bracts around a small flower core."""
    normal = Vector((0.0, 0.0, 1.0))
    core = [bm.verts.new(centre + direction * 0.016) for direction in (
        Vector((1, 0, 0)), Vector((-1, 0, 0)), Vector((0, 1, 0)),
        Vector((0, -1, 0)), Vector((0, 0, 1)), Vector((0, 0, -1)),
    )]
    for face in ((4, 0, 2), (4, 2, 1), (4, 1, 3), (4, 3, 0),
                 (5, 2, 0), (5, 1, 2), (5, 3, 1), (5, 0, 3)):
        bm.faces.new(tuple(core[index] for index in face))

    for index in range(BRACTS_PER_HEAD):
        z = 1.0 - 2.0 * (index + 0.5) / BRACTS_PER_HEAD
        angle = phase + index * 2.399963229728653
        radius = math.sqrt(max(0.0, 1.0 - z * z))
        outward = Vector((math.cos(angle) * radius, math.sin(angle) * radius, z)).normalized()
        tangent = normal.cross(outward)
        if tangent.length_squared < 1e-6:
            tangent = Vector((1.0, 0.0, 0.0))
        tangent.normalize()
        side = outward.cross(tangent).normalized()
        root = centre + outward * 0.010
        shoulder = centre + outward * 0.020
        tip = centre + outward * (0.038 + 0.004 * (index % 3))
        half_width = 0.010 + 0.001 * (index % 2)
        root_v = bm.verts.new(root)
        left_v = bm.verts.new(shoulder + tangent * half_width)
        right_v = bm.verts.new(shoulder - tangent * half_width)
        tip_v = bm.verts.new(tip)
        bm.faces.new((root_v, left_v, tip_v))
        bm.faces.new((root_v, tip_v, right_v))
        # A subtle ridge keeps each bract readable in a close preview.
        ridge = bm.verts.new(shoulder + side * 0.0015)
        bm.faces.new((root_v, tip_v, ridge))


def build(A):
    lib.PLAIN["sea_holly_leaf"] = {"BaseColor": [0.30, 0.39, 0.34], "Roughness": 0.93}
    lib.PLAIN["sea_holly_stem"] = {"BaseColor": [0.34, 0.41, 0.34], "Roughness": 0.91}
    lib.PLAIN["sea_holly_bract"] = {"BaseColor": [0.27, 0.42, 0.49], "Roughness": 0.86}

    leaf_bm = bmesh.new()
    bract_bm = bmesh.new()

    crown = Vector((0.0, 0.0, 0.008))
    for leaf_index in range(ROSETTE_LEAVES):
        angle = math.tau * leaf_index / ROSETTE_LEAVES + 0.13
        origin = crown + Vector((0.012 * math.cos(angle), 0.012 * math.sin(angle), 0.0))
        _leaf(leaf_bm, origin, angle, LEAF_LENGTHS[leaf_index], angle * 0.43)

    for head_index, (x, y, height) in enumerate(FLOWER_HEADS):
        base = Vector((x * 0.45, y * 0.45, 0.012))
        head = Vector((x, y, height))
        sway = Vector((0.012 * math.sin(head_index * 1.7),
                       0.010 * math.cos(head_index * 1.3), 0.0))
        top = head + sway
        A.tube("sea_holly_stem", [base, base.lerp(top, 0.50), top],
               0.0042, segs=5, taper=lambda t: 1.0 - 0.30 * t,
               name="seaHollyFlowerScape{:02d}".format(head_index + 1))
        _bract_head(bract_bm, top, head_index * 0.71)

    if leaf_bm.faces:
        bmesh.ops.solidify(leaf_bm, geom=leaf_bm.faces[:], thickness=0.0018)
        A._finish(leaf_bm, "seaHollySpinyRosette", "sea_holly_leaf", (0, 0, 0),
                  (0, 0, 0), "box", "Z", 1.0, True, 50)
    else:
        leaf_bm.free()

    if bract_bm.faces:
        A._finish(bract_bm, "seaHollyBlueBracts", "sea_holly_bract", (0, 0, 0),
                  (0, 0, 0), "box", "Z", 1.0, True, 50)
    else:
        bract_bm.free()

    # Static ground-cover instances must not snag the player or affect navigation.
    for obj, _material in A.parts:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(obj.data)
        bm.free()
