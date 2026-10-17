"""An upright soft-rush clump for damp ground at the Tideglass waterline."""
import math

import bmesh
from mathutils import Vector

import lib


NAME = "TideglassSoftRush"
PROVENANCE = "Collisionless procedural geometry authored in Blender 5.2; two flat-color material slots; no generated textures."
STEM_BASES = (
    (-0.10, -0.07), (-0.05, 0.09), (0.02, -0.11), (0.08, 0.05),
    (0.13, -0.02), (-0.13, 0.03), (-0.08, 0.14), (0.05, 0.13),
    (0.16, 0.08), (0.01, 0.02), (-0.17, -0.06), (0.11, -0.13),
    (-0.02, -0.17), (0.17, -0.10), (-0.13, 0.12), (0.07, -0.01),
)
STEM_HEIGHTS = (0.66, 0.78, 0.59, 0.86, 0.72, 0.63, 0.82, 0.69,
                0.57, 0.91, 0.74, 0.61, 0.80, 0.68, 0.55, 0.76)
MIN_CULM_LEAN = 0.025
MAX_CULM_LEAN = 0.065
MID_STEM_CURVE = 0.012
SEED_BRANCHES = 3
SPIKELETS_PER_BRANCH = 3


def _seed_pods(bm, centre, direction, phase):
    axis = Vector(direction).normalized()
    side = Vector((-axis.y, axis.x, 0.0))
    for pod_index in range(SPIKELETS_PER_BRANCH):
        angle = phase + math.tau * pod_index / SPIKELETS_PER_BRANCH
        radial = axis * math.cos(angle) + side * math.sin(angle)
        pod_centre = centre + radial * 0.008
        result = bmesh.ops.create_uvsphere(
            bm, u_segments=5, v_segments=3, radius=1.0, calc_uvs=False)
        for vert in result["verts"]:
            vert.co.x = pod_centre.x + vert.co.x * 0.0042
            vert.co.y = pod_centre.y + vert.co.y * 0.0042
            vert.co.z = pod_centre.z + vert.co.z * 0.0070


def build(A):
    lib.PLAIN["soft_rush_stem"] = {"BaseColor": [0.25, 0.34, 0.23], "Roughness": 0.92}
    lib.PLAIN["soft_rush_seed"] = {"BaseColor": [0.40, 0.31, 0.18], "Roughness": 0.89}

    seed_bm = bmesh.new()
    for stem_index, ((x, y), height) in enumerate(zip(STEM_BASES, STEM_HEIGHTS)):
        base = Vector((x, y, 0.004))
        lean_angle = stem_index * 2.399963229728653
        lean_length = MIN_CULM_LEAN + (MAX_CULM_LEAN - MIN_CULM_LEAN) * (stem_index % 5) / 4.0
        lean = Vector((lean_length * math.cos(lean_angle),
                       lean_length * math.sin(lean_angle), 0.0))
        curve = Vector((MID_STEM_CURVE * math.cos(lean_angle + 1.1),
                        MID_STEM_CURVE * math.sin(lean_angle + 1.1), 0.0))
        top = base + lean + Vector((0.0, 0.0, height))
        path = [base.lerp(top, step / 6.0)
                + curve * math.sin(math.pi * step / 6.0)
                + Vector((0.0, 0.0, 0.006 * math.sin(math.pi * step / 6.0)))
                for step in range(7)]
        A.tube("soft_rush_stem", path, 0.0048, segs=6,
               taper=lambda t: 1.0 - 0.20 * t,
               name="softRushCulm{:02d}".format(stem_index + 1))

        # Soft rush keeps its small seed cluster on a short side branch near the tip.
        branch_point = base.lerp(top, 0.80)
        side_angle = lean_angle + 0.48 * math.sin(stem_index * 1.13)
        side_axis = Vector((math.cos(side_angle), math.sin(side_angle), 0.0))
        branch_root = branch_point + side_axis * 0.008
        for branch_index in range(SEED_BRANCHES):
            spread = (branch_index - (SEED_BRANCHES - 1) / 2.0) / 2.0
            tangent = Vector((-side_axis.y, side_axis.x, 0.0))
            branch_direction = (side_axis + tangent * (spread * 0.42)
                                + Vector((0.0, 0.0, -0.28 - 0.035 * (branch_index % 2))))
            branch_length = 0.043 + 0.009 * ((stem_index + branch_index) % 4)
            branch_tip = branch_root + branch_direction.normalized() * branch_length
            A.tube("soft_rush_stem", (branch_root, branch_tip), 0.0018,
                   segs=5, taper=lambda t: 1.0 - 0.55 * t,
                   name="softRushPanicle{:02d}_{:02d}".format(stem_index + 1, branch_index + 1))
            _seed_pods(seed_bm, branch_tip, branch_direction,
                       stem_index * 0.31 + branch_index * 0.73)

    if seed_bm.faces:
        A._finish(seed_bm, "softRushSeedPods", "soft_rush_seed", (0, 0, 0),
                  (0, 0, 0), "box", "Z", 1.0, True, 48)
    else:
        seed_bm.free()

    # No collision is authored: this remains safe to trial as non-blocking foliage.
    for obj, _material in A.parts:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(obj.data)
        bm.free()
