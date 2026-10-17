"""An airy common sea-lavender clump for the upper salt-meadow transition."""
import math

import bmesh
from mathutils import Vector

import lib


NAME = "CoastalSeaLavender"
PROVENANCE = "Collisionless Limonium vulgare-inspired geometry procedurally authored in Blender 5.2; three flat-color material slots; no generated textures."
ROSETTE_CENTRES = (
    (-0.11, -0.04), (-0.045, 0.12), (0.035, -0.14),
    (0.12, 0.045), (-0.015, 0.005), (0.17, -0.09),
)
ROSETTE_LEAVES = 6
FLOWER_STEMS = (
    (-0.12, -0.06, 0.49), (-0.07, 0.11, 0.44), (0.015, -0.12, 0.53),
    (0.06, 0.08, 0.46), (0.13, 0.02, 0.55), (0.17, -0.10, 0.47),
    (-0.015, 0.02, 0.58), (0.045, -0.045, 0.50),
)
BRANCHES_PER_STEM = 3
FLOWERS_PER_BRANCH = 4
PETALS_PER_FLOWER = 5


def _rosette_leaf(bm, origin, angle, length, width, phase):
    """A fleshy, tapering basal leaf with a shallow cupped cross-section."""
    direction = Vector((math.cos(angle), math.sin(angle), 0.0))
    across = Vector((-direction.y, direction.x, 0.0))
    rows = []
    for row in range(7):
        t = row / 6.0
        centre = origin + direction * (length * t)
        centre += across * (0.012 * math.sin(math.pi * t + phase))
        centre.z += 0.006 + 0.055 * t + 0.018 * math.sin(math.pi * t + phase)
        half_width = width * math.sin(math.pi * t) ** 0.72
        vertices = []
        for side in (-1.0, 0.0, 1.0):
            point = centre + across * (side * half_width)
            point.z += 0.008 * (1.0 - side * side) * math.sin(math.pi * t)
            vertices.append(bm.verts.new(point))
        rows.append(vertices)
    for row in range(6):
        for column in range(2):
            bm.faces.new((rows[row][column], rows[row + 1][column],
                          rows[row + 1][column + 1], rows[row][column + 1]))


def _star_floret(bm, centre, direction, size, phase):
    """A tiny five-petal star, kept as shared low-poly geometry."""
    normal = Vector(direction).normalized()
    across = Vector((0.0, 0.0, 1.0)).cross(normal)
    if across.length_squared < 1e-6:
        across = Vector((1.0, 0.0, 0.0))
    across.normalize()
    side = normal.cross(across).normalized()
    for petal in range(PETALS_PER_FLOWER):
        angle = phase + math.tau * petal / PETALS_PER_FLOWER
        radial = across * math.cos(angle) + side * math.sin(angle)
        tangent = -across * math.sin(angle) + side * math.cos(angle)
        root = centre + normal * (size * 0.12) + radial * (size * 0.18)
        tip = centre + normal * (size * 0.3) + radial * size
        half_width = size * 0.26
        left = root + tangent * half_width
        right = root - tangent * half_width
        bm.faces.new((bm.verts.new(left), bm.verts.new(tip),
                      bm.verts.new(right), bm.verts.new(root)))


def build(A):
    # Unreal's first real-RHI trial washed the basal leaves toward grey and
    # lost the sub-centimetre stems/flowers at play distance. A deeper sage
    # leaf and slightly brighter, thicker inflorescence are a measured art
    # iteration, not a final Island material approval.
    lib.PLAIN["sea_lavender_leaf"] = {"BaseColor": [0.16, 0.28, 0.17], "Roughness": 0.94}
    lib.PLAIN["sea_lavender_stem"] = {"BaseColor": [0.43, 0.50, 0.28], "Roughness": 0.92}
    lib.PLAIN["sea_lavender_flower"] = {"BaseColor": [0.70, 0.58, 0.88], "Roughness": 0.84}

    leaf_bm = bmesh.new()
    flower_bm = bmesh.new()

    # A low grey-green rosette anchors the taller, airy flower sprays.
    for plant_index, (x, y) in enumerate(ROSETTE_CENTRES):
        origin = Vector((x, y, 0.002))
        for leaf_index in range(ROSETTE_LEAVES):
            angle = math.tau * leaf_index / ROSETTE_LEAVES + plant_index * 0.29
            _rosette_leaf(leaf_bm, origin, angle,
                          0.19 + 0.035 * ((plant_index + leaf_index) % 3),
                          0.028 + 0.004 * ((plant_index + 2 * leaf_index) % 3),
                          angle * 0.41)

    # Each stem forks into a sparse upright spray. Thicker export-safe rods
    # and larger stars keep the cluster legible without making a purple pom-pom.
    for stem_index, (x, y, height) in enumerate(FLOWER_STEMS):
        base = Vector((x, y, 0.015))
        top = Vector((x + 0.025 * math.sin(stem_index),
                      y + 0.022 * math.cos(stem_index * 0.7), height))
        A.tube("sea_lavender_stem", [base, base.lerp(top, 0.52), top],
               0.0042, segs=5, taper=lambda t: 1.0 - 0.45 * t,
               name="seaLavenderStem{:02d}".format(stem_index + 1))
        for branch_index in range(BRANCHES_PER_STEM):
            t = 0.37 + 0.14 * branch_index
            anchor = base.lerp(top, t)
            angle = stem_index * 2.399963229728653 + branch_index * math.tau / BRANCHES_PER_STEM
            radial = Vector((math.cos(angle), math.sin(angle), 0.0))
            branch_end = anchor + radial * (0.09 + 0.012 * (branch_index % 2))
            branch_end.z += 0.045 + 0.012 * ((stem_index + branch_index) % 3)
            A.tube("sea_lavender_stem", [anchor, anchor.lerp(branch_end, 0.55), branch_end],
                   0.0026, segs=4, taper=lambda t: 1.0 - 0.62 * t,
                   name="seaLavenderBranch{:02d}_{:02d}".format(stem_index + 1, branch_index + 1))
            branch_axis = (branch_end - anchor).normalized()
            for flower_index in range(FLOWERS_PER_BRANCH):
                flower_t = 0.14 + flower_index * 0.23
                flower_centre = anchor.lerp(branch_end, flower_t)
                flower_centre.z += 0.009 * math.sin(flower_index * 1.7 + stem_index)
                _star_floret(flower_bm, flower_centre, branch_axis,
                             0.014 + 0.002 * ((stem_index + flower_index) % 3),
                             angle + flower_index * 0.61)

    if leaf_bm.faces:
        bmesh.ops.solidify(leaf_bm, geom=leaf_bm.faces[:], thickness=0.0015)
        A._finish(leaf_bm, "seaLavenderBasalLeaves", "sea_lavender_leaf", (0, 0, 0),
                  (0, 0, 0), "box", "Z", 1.0, True, 48)
    else:
        leaf_bm.free()

    if flower_bm.faces:
        A._finish(flower_bm, "seaLavenderStarFlowers", "sea_lavender_flower", (0, 0, 0),
                  (0, 0, 0), "box", "Z", 1.0, True, 48)
    else:
        flower_bm.free()

    # Stems are separate small meshes from A.tube; all remain collisionless.
    for obj, _material in A.parts:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(obj.data)
        bm.free()
