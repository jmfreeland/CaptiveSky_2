"""A low, fleshy rock-samphire clump for Tideglass's spray-wet stone margins."""
import math

import bmesh
from mathutils import Vector

import lib


NAME = "TideglassRockSamphire"
PROVENANCE = "Deterministic collisionless Crithmum maritimum-inspired procedural geometry authored in Blender 5.2; three flat-color material slots; no generated textures."
STEM_BASES = (
    (-0.13, -0.04, 0.006), (-0.05, 0.14, 0.006), (0.11, 0.09, 0.006),
    (0.12, -0.10, 0.006), (-0.08, -0.11, 0.006),
)
STEM_HEIGHTS = (0.30, 0.36, 0.33, 0.39, 0.31)
BRANCHES_PER_STEM = 3
LEAF_NODES_PER_BRANCH = 2
LEAFLETS_PER_NODE = 3
FLORETS_PER_UMBEL = 7


def _umbel_floret(bm, position, phase):
    """A small five-petal flower; many grouped together make a flat umbel."""
    centre = Vector(position)
    for petal in range(5):
        angle = phase + math.tau * petal / 5.0
        radial = Vector((math.cos(angle), math.sin(angle), 0.0))
        tangent = Vector((-radial.y, radial.x, 0.0))
        root = centre + radial * 0.001
        shoulder = centre + radial * 0.0025 + Vector((0.0, 0.0, 0.001))
        tip = centre + radial * 0.0055 + Vector((0.0, 0.0, 0.002))
        left = shoulder - tangent * 0.0015
        right = shoulder + tangent * 0.0015
        vertices = [bm.verts.new(point) for point in (root, left, tip, right)]
        bm.faces.new((vertices[0], vertices[1], vertices[2]))
        bm.faces.new((vertices[0], vertices[2], vertices[3]))


def build(A):
    lib.PLAIN["samphire_stem"] = {"BaseColor": [0.16, 0.27, 0.15], "Roughness": 0.92}
    lib.PLAIN["samphire_leaf"] = {"BaseColor": [0.20, 0.34, 0.19], "Roughness": 0.88}
    lib.PLAIN["samphire_flower"] = {"BaseColor": [0.70, 0.43, 0.10], "Roughness": 0.86}

    flower_bm = bmesh.new()

    for stem_index, (base_tuple, height) in enumerate(zip(STEM_BASES, STEM_HEIGHTS)):
        base = Vector(base_tuple)
        angle = math.tau * stem_index / len(STEM_BASES) + 0.17 * (stem_index % 2)
        radial = Vector((math.cos(angle), math.sin(angle), 0.0))
        top = base + radial * (0.052 + 0.008 * (stem_index % 3)) + Vector((0.0, 0.0, height))
        path = [base.lerp(top, step / 5.0)
                + radial * (0.018 * math.sin(math.pi * step / 5.0))
                for step in range(6)]
        A.tube("samphire_stem", path, 0.0040, segs=5,
               taper=lambda t: 1.0 - 0.25 * t,
               name="samphireScape{:02d}".format(stem_index + 1))

        # Short lateral branches break the upright scapes into a loose coastal clump.
        for branch_index in range(BRANCHES_PER_STEM):
            branch_t = 0.16 + 0.18 * branch_index
            branch_start = base.lerp(top, branch_t)
            branch_angle = angle + (branch_index - 1) * 0.72
            branch_radial = Vector((math.cos(branch_angle), math.sin(branch_angle), 0.0))
            branch_length = 0.078 - 0.008 * abs(branch_index - 1)
            branch_top = branch_start + branch_radial * branch_length + Vector((0.0, 0.0, 0.046))
            branch_path = [branch_start,
                           branch_start.lerp(branch_top, 0.5) + Vector((0.0, 0.0, 0.006)),
                           branch_top]
            A.tube("samphire_stem", branch_path, 0.0028, segs=5,
                   taper=lambda t: 1.0 - 0.45 * t,
                   name="samphireBranch{:02d}_{:02d}".format(stem_index + 1, branch_index + 1))

            branch_direction = (branch_top - branch_start).normalized()
            for node_index in range(LEAF_NODES_PER_BRANCH):
                node = branch_start.lerp(branch_top, 0.34 + 0.34 * node_index)
                for leaf_index in range(LEAFLETS_PER_NODE):
                    leaf_angle = branch_angle + math.tau * leaf_index / LEAFLETS_PER_NODE + node_index * 0.31
                    leaf_radial = Vector((math.cos(leaf_angle), math.sin(leaf_angle), 0.0))
                    leaf_axis = (branch_direction * 0.25 + leaf_radial * 0.76
                                 + Vector((0.0, 0.0, 0.48))).normalized()
                    leaf_tip = node + leaf_axis * (0.050 - 0.006 * node_index)
                    leaf_path = [node,
                                 node.lerp(leaf_tip, 0.52) + leaf_radial * 0.006
                                 + Vector((0.0, 0.0, 0.004)), leaf_tip]
                    A.tube("samphire_leaf", leaf_path, 0.0058, segs=5,
                           taper=lambda t: 0.95 - 0.53 * t,
                           name="samphireLeaf{:02d}_{:02d}_{:02d}".format(
                               stem_index + 1, branch_index + 1, node_index * LEAFLETS_PER_NODE + leaf_index + 1))

            # Each branch carries one small umbel, breaking up the tall silhouette.
            umbel_centre = branch_top + Vector((0.0, 0.0, 0.006))
            for floret_index in range(FLORETS_PER_UMBEL):
                fraction = (floret_index + 0.5) / FLORETS_PER_UMBEL
                flower_angle = floret_index * 2.399963229728653 + stem_index * 0.29 + branch_index
                radius = 0.011 * math.sqrt(fraction)
                position = umbel_centre + Vector((math.cos(flower_angle) * radius,
                                                   math.sin(flower_angle) * radius,
                                                   0.002 * (1.0 - fraction)))
                _umbel_floret(flower_bm, position, flower_angle * 0.37)

        # A smaller terminal umbel crowns each main scape.
        umbel_centre = top + Vector((0.0, 0.0, 0.008))
        for floret_index in range(FLORETS_PER_UMBEL):
            fraction = (floret_index + 0.5) / FLORETS_PER_UMBEL
            flower_angle = floret_index * 2.399963229728653 + stem_index * 0.29
            radius = 0.010 * math.sqrt(fraction)
            position = umbel_centre + Vector((math.cos(flower_angle) * radius,
                                               math.sin(flower_angle) * radius,
                                               0.002 * (1.0 - fraction)))
            _umbel_floret(flower_bm, position, flower_angle * 0.37)

    if flower_bm.faces:
        A._finish(flower_bm, "rockSamphireUmbels", "samphire_flower", (0, 0, 0), (0, 0, 0),
                  "box", "Z", 1.0, True, 48)
    else:
        flower_bm.free()

    # Repeated coastal ground cover is static, collisionless, and non-navigable.
    for obj, _material in A.parts:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(obj.data)
        bm.free()
