"""A wind-combed marram-grass clump for the exposed Tideglass dunes."""
import math

import bmesh
from mathutils import Vector

import lib


NAME = "TideglassMarramGrass"
PROVENANCE = "Geometry procedurally authored in Blender 5.2; three flat-color material slots; no generated textures."
WIND_LEAN = (-0.19, 0.045, 0.0)
LEAF_COUNT = 9
LEAF_LENGTHS = (0.53, 0.64, 0.48, 0.68, 0.57, 0.45, 0.61, 0.50, 0.66)
LEAF_WIDTHS = (0.017, 0.020, 0.014, 0.019, 0.016, 0.013, 0.018, 0.015, 0.018)
SEED_HEADS = (
    ((-0.045, -0.035, 0.025), 0.73, -0.34),
    ((0.030, 0.018, 0.025), 0.65, -0.20),
    ((0.060, -0.025, 0.025), 0.69, -0.28),
)
SPIKELETS_PER_HEAD = 36


def _blade(bm, origin, angle, length, width, phase):
    direction = Vector((math.cos(angle), math.sin(angle), 0.0))
    across = Vector((-direction.y, direction.x, 0.0))
    wind_lean = Vector(WIND_LEAN)
    rows = []
    for row in range(11):
        t = row / 10.0
        # Marram leaves lift from a tight basal crown then comb over in the wind.
        centre = origin + direction * (length * (0.20 * t + 0.73 * t * t))
        centre += wind_lean * (length * t ** 2.1)
        centre += across * (0.012 * math.sin(math.pi * t + phase) * t)
        centre.z += length * (0.78 * t - 0.32 * t * t) + 0.004
        profile = math.sin(math.pi * min(0.999, t)) ** 0.52
        half_width = width * profile * (0.72 + 0.28 * (1.0 - t))
        row_verts = []
        for side in (-1.0, 0.0, 1.0):
            point = centre + across * (side * half_width)
            # A shallow central fold catches light without making a flat card.
            point.z += 0.0045 * (1.0 - side * side) * profile
            row_verts.append(bm.verts.new(point))
        rows.append(row_verts)
    for row in range(10):
        for column in range(2):
            bm.faces.new((rows[row][column], rows[row + 1][column],
                          rows[row + 1][column + 1], rows[row][column + 1]))


def _seed_spikelets(A, head_index):
    base, height, lean = SEED_HEADS[head_index]
    root = Vector(base)
    wind_lean = Vector(WIND_LEAN)
    stalk_tip = root + Vector((lean, 0.0, height)) + wind_lean * (height * 0.55)
    stalk_path = [root.lerp(stalk_tip, step / 6.0)
                  + Vector((0.0, 0.0, 0.006 * math.sin(math.pi * step / 6.0)))
                  for step in range(7)]
    A.tube("marram_stem", stalk_path, 0.0027, segs=5,
           taper=lambda t: 1.0 - 0.26 * t,
           name="marramSeedStalk{:02d}".format(head_index + 1))

    spike_axis = (stalk_tip - root).normalized()
    across = spike_axis.cross(Vector((0.0, 1.0, 0.0))).normalized()
    if across.length_squared < 1e-6:
        across = Vector((1.0, 0.0, 0.0))
    side = spike_axis.cross(across).normalized()
    for spike_index in range(SPIKELETS_PER_HEAD):
        t = (spike_index + 0.5) / SPIKELETS_PER_HEAD
        centre = root.lerp(stalk_tip, 0.29 + 0.54 * t)
        ring_angle = spike_index * 2.399963229728653 + head_index * 0.71
        radial = across * math.cos(ring_angle) + side * math.sin(ring_angle)
        centre += radial * (0.018 * math.sin(math.pi * t))
        length = 0.036 + 0.010 * ((spike_index + head_index) % 4) / 3.0
        tip = centre + radial * length * 0.62 - spike_axis * length * 0.50
        tip += wind_lean * (0.020 * t)
        A.tube("marram_seed", (centre, tip), 0.0038, segs=5,
               taper=lambda u: 1.0 - 0.84 * u,
               name="marramSpikelet{:02d}_{:02d}".format(head_index + 1, spike_index + 1))


def build(A):
    lib.PLAIN["marram_leaf"] = {"BaseColor": [0.25, 0.34, 0.22], "Roughness": 0.91}
    lib.PLAIN["marram_stem"] = {"BaseColor": [0.32, 0.37, 0.25], "Roughness": 0.89}
    lib.PLAIN["marram_seed"] = {"BaseColor": [0.68, 0.56, 0.34], "Roughness": 0.86}

    leaves = bmesh.new()
    for leaf_index in range(LEAF_COUNT):
        angle = leaf_index * 2.399963229728653
        base = Vector((0.018 * math.cos(angle * 1.3),
                       0.016 * math.sin(angle * 1.7), 0.008))
        _blade(leaves, base, angle, LEAF_LENGTHS[leaf_index],
               LEAF_WIDTHS[leaf_index], angle * 0.41)
    if leaves.faces:
        bmesh.ops.solidify(leaves, geom=leaves.faces[:], thickness=0.0016)
        A._finish(leaves, "marramBlades", "marram_leaf", (0, 0, 0), (0, 0, 0),
                  "box", "Z", 1.0, True, 48)
    else:
        leaves.free()

    for head_index in range(len(SEED_HEADS)):
        _seed_spikelets(A, head_index)

    # Instances are intentionally collisionless; the clump is visual groundcover.
    for obj, _material in A.parts:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(obj.data)
        bm.free()
