"""A low woodland wild-strawberry groundcover patch with trifoliate leaves, spring flowers, and ripe fruit."""
import math

import bmesh
from mathutils import Vector

import lib


NAME = "WoodlandWildStrawberry"
LEAF_COUNTS = (4, 4, 4, 3)
FLOWER_POSITIONS = (
    (-0.25, -0.12, 0.218), (0.16, -0.20, 0.226), (-0.14, 0.20, 0.214),
    (0.28, 0.18, 0.220), (0.03, 0.02, 0.204),
)
BERRY_POSITIONS = (
    (-0.28, -0.20, 0.050), (0.22, -0.25, 0.047), (-0.20, 0.25, 0.052), (0.30, 0.22, 0.048),
)


def _leaflet(bm, origin, angle, length, width, phase):
    direction = Vector((math.cos(angle), math.sin(angle), 0.0))
    across = Vector((-direction.y, direction.x, 0.0))
    rows = []
    for row in range(7):
        t = row / 6
        # A softly cupped, broad leaflet with subtle strawberry-like edge teeth.
        half_width = width * (math.sin(math.pi * t) ** 0.86) * (0.96 - 0.12 * t)
        centre = origin + direction * (length * t)
        centre.z += 0.017 * math.sin(math.pi * t) + 0.0025 * math.sin(3.0 * t * math.pi + phase)
        edge_scale = 0.94 if row in (2, 4) else 1.0
        left = centre - across * (half_width * edge_scale)
        right = centre + across * (half_width * edge_scale)
        left.z += 0.004 * math.sin(math.pi * t)
        right.z += 0.004 * math.sin(math.pi * t)
        rows.append((bm.verts.new(left), bm.verts.new(centre), bm.verts.new(right)))
    for row in range(6):
        for side in range(2):
            bm.faces.new((rows[row][side], rows[row + 1][side],
                          rows[row + 1][side + 1], rows[row][side + 1]))


def _compound_leaf(A, leaf_bm, center, angle, scale, index):
    petiole_length = 0.13 * scale
    petiole_end = center + Vector((math.cos(angle), math.sin(angle), 0.0)) * petiole_length
    petiole_end.z += 0.035 * scale
    A.tube("strawberry_stem", [center, center.lerp(petiole_end, 0.5) + Vector((0, 0, 0.015)), petiole_end],
           0.0038 * scale, segs=5, taper=lambda t: 1.0 - 0.28 * t,
           name="leafPetiole{:02d}".format(index))

    # The familiar three-part leaf: one longer terminal leaflet and two splayed sides.
    central = petiole_end + Vector((math.cos(angle), math.sin(angle), 0.0)) * (0.055 * scale)
    _leaflet(leaf_bm, petiole_end, angle, 0.178 * scale, 0.074 * scale, index * 0.7)
    for side in (-1, 1):
        side_angle = angle + side * 0.93
        attachment = petiole_end + Vector((math.cos(side_angle), math.sin(side_angle), 0.0)) * (0.012 * scale)
        _leaflet(leaf_bm, attachment, side_angle, 0.146 * scale, 0.062 * scale, index * 0.7 + side)
    A.tube("strawberry_stem", [petiole_end, central], 0.0028 * scale, segs=5,
           taper=lambda t: 1.0 - 0.38 * t, name="leafMidrib{:02d}".format(index))


def _flower(A, center, index):
    A.tube("strawberry_stem", [center + Vector((0, 0, -0.075)),
                                center + Vector((0.008 * (index - 1), 0, -0.028)), center],
           0.0032, segs=5, taper=lambda t: 1.0 - 0.3 * t, name="flowerStem{:02d}".format(index))
    petal_bm = bmesh.new()
    for petal in range(5):
        angle = math.tau * petal / 5 + index * 0.23
        radial = Vector((math.cos(angle), math.sin(angle), 0.0))
        tangent = Vector((-radial.y, radial.x, 0.0))
        base = center + radial * 0.003
        tip = center + radial * 0.031
        left = base + tangent * 0.007
        right = base - tangent * 0.007
        peak = center + radial * 0.020 + Vector((0, 0, 0.006))
        petal_bm.faces.new((petal_bm.verts.new(left), petal_bm.verts.new(peak),
                            petal_bm.verts.new(tip), petal_bm.verts.new(right)))
    bmesh.ops.solidify(petal_bm, geom=petal_bm.faces[:], thickness=0.0012)
    A._finish(petal_bm, "strawberryFlower{:02d}".format(index), "strawberry_flower",
              (0, 0, 0), (0, 0, 0), "box", "Z", 1.0, True, 48)
    A.rock("strawberry_pollen", (0.006, 0.006, 0.004), loc=center + Vector((0, 0, 0.003)),
           seed=170 + index, sub=1, rough=0.06, name="flowerCentre{:02d}".format(index))


def build(A):
    lib.PLAIN["strawberry_leaf"] = {"BaseColor": [0.14, 0.27, 0.085], "Roughness": 0.84}
    lib.PLAIN["strawberry_stem"] = {"BaseColor": [0.22, 0.32, 0.12], "Roughness": 0.88}
    lib.PLAIN["strawberry_flower"] = {"BaseColor": [0.82, 0.79, 0.66], "Roughness": 0.78}
    lib.PLAIN["strawberry_pollen"] = {"BaseColor": [0.90, 0.58, 0.10], "Roughness": 0.72}
    lib.PLAIN["strawberry_fruit"] = {"BaseColor": [0.48, 0.045, 0.025], "Roughness": 0.56}

    # Four low rosette groups make a readable patch while leaving gaps for soil and litter.
    leaf_bm = bmesh.new()
    leaf_groups = (
        ((-0.24, -0.15, 0.020), 0.00, 1.00),
        ((0.23, -0.20, 0.018), 1.75, 0.90),
        ((-0.19, 0.21, 0.017), 3.70, 0.86),
        ((0.25, 0.20, 0.019), 0.88, 0.92),
    )
    index = 0
    for group_index, (position, phase, scale) in enumerate(leaf_groups):
        origin = Vector(position)
        for leaf in range(LEAF_COUNTS[group_index]):
            angle = phase + math.tau * leaf / LEAF_COUNTS[group_index] + (0.12 * group_index)
            _compound_leaf(A, leaf_bm, origin, angle, scale, index)
            index += 1

    if leaf_bm.faces:
        bmesh.ops.solidify(leaf_bm, geom=leaf_bm.faces[:], thickness=0.0014)
        A._finish(leaf_bm, "strawberryLeaves", "strawberry_leaf", (0, 0, 0), (0, 0, 0),
                  "box", "Z", 1.0, True, 50)
    else:
        leaf_bm.free()

    for flower_index, position in enumerate(FLOWER_POSITIONS, 1):
        _flower(A, Vector(position), flower_index)

    for berry_index, position in enumerate(BERRY_POSITIONS, 1):
        A.rock("strawberry_fruit", (0.027, 0.023, 0.024), loc=position,
               seed=270 + berry_index, sub=1, rough=0.035, flat_bottom=0.15,
               name="ripeBerry{:02d}".format(berry_index))

    # Groundcover is deliberately non-colliding so it cannot snag a walking character.
    for obj, _material in A.parts:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(obj.data)
        bm.free()
