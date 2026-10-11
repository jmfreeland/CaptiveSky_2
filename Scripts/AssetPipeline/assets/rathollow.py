"""A weathered fallen trunk hollow with a rat-sized entrance and a sheltered nest bed."""
import math

import bmesh
from mathutils import Vector


NAME = "RatHollow_TornEntranceV2"
ENTRANCE_BARK_BREAK_SEGMENTS = (9, 10)
ENTRANCE_RIM_IRREGULARITY = 1.35


def _ring(bm, x, radius, center_z, phase, segments, center_y=0.0, irregularity=1.0):
    verts = []
    for i in range(segments):
        angle = 2.0 * math.pi * i / segments
        # A few broad, offset deformations keep the trunk from reading as a
        # manufactured cylinder; the small high-frequency term breaks the rim.
        variation = (1.0 + irregularity * (
            0.060 * math.sin(3.0 * angle + phase)
            + 0.026 * math.sin(7.0 * angle - phase * 0.7)
            + 0.022 * math.sin(2.0 * angle + phase * 1.3)))
        x_jitter = 0.024 * math.sin(5.0 * angle + phase)
        r = radius * variation
        verts.append(bm.verts.new((x + x_jitter, center_y + r * math.cos(angle), center_z + r * math.sin(angle))))
    return verts


def _bridge(bm, first, second, skip_segments=()):
    for i in range(len(first)):
        if i in skip_segments:
            continue
        j = (i + 1) % len(first)
        bm.faces.new((first[i], first[j], second[j], second[i]))


def _cap(bm, ring, x, center_z):
    center = bm.verts.new((x, 0.0, center_z))
    for i in range(len(ring)):
        bm.faces.new((center, ring[i], ring[(i + 1) % len(ring)]))


def _hollow_log(A):
    segments = 40
    center_z = 0.66
    bm = bmesh.new()

    outer_stations = [
        (-1.30, 0.57, 0.65), (-1.17, 0.62, 0.66), (-0.88, 0.67, 0.67),
        (-0.48, 0.65, 0.66), (0.05, 0.63, 0.66), (0.55, 0.61, 0.65),
        (1.00, 0.58, 0.65), (1.24, 0.52, 0.65),
    ]
    outer = [_ring(bm, x, radius, z, k * 0.61, segments, 0.035 * math.sin(k * 0.83))
             for k, (x, radius, z) in enumerate(outer_stations)]
    for first, second in zip(outer, outer[1:]):
        _bridge(bm, first, second)
    _cap(bm, outer[-1], outer_stations[-1][0], outer_stations[-1][2])

    # An annular broken rim joins the outside bark to the open entrance.
    inner_stations = [
        (-1.17, 0.39, 0.66), (-0.92, 0.39, 0.66), (-0.54, 0.38, 0.66),
        (-0.12, 0.37, 0.66), (0.28, 0.34, 0.66), (0.45, 0.30, 0.66),
    ]
    inner = [_ring(bm, x, radius, z, -k * 0.29, segments,
                   irregularity=ENTRANCE_RIM_IRREGULARITY if k == 0 else 1.0)
             for k, (x, radius, z) in enumerate(inner_stations)]
    # A small, off-centre bite through the top of the annular rim breaks the
    # manufactured ring without closing or narrowing the rat-sized passage.
    _bridge(bm, outer[1], inner[0], ENTRANCE_BARK_BREAK_SEGMENTS)
    for first, second in zip(inner, inner[1:]):
        _bridge(bm, first, second)
    # Keep the tunnel open to the dark backing below; a wood cap here hides the cavity.

    A._finish(bm, "hollowBark", "wood", (0, 0, 0), (0, 0, 0), "box", "X", 1.0, True, 55)

    # A dark inner end and a little dry nest lining make the opening legible at game scale.
    # Keep the recess dark, but let warm brown earth tone and ambient light
    # reveal its depth instead of reading as a featureless black disc.
    A.cyl("dirt", 0.29, 0.018, loc=(0.475, 0, center_z), rot=(0, 90, 0),
          segs=40, uv="box", name="hollowDepth")
    A.rock("straw", (0.34, 0.27, 0.060), loc=(-0.32, 0, 0.325), seed=31,
           sub=2, rough=0.06, flat_bottom=0.6, tile=0.4, name="nestBed")
    for i, (x, y) in enumerate(((-0.55, -0.10), (-0.18, 0.16), (0.12, -0.14))):
        A.tube("straw", [Vector((x - 0.22, y - 0.05, 0.31)), Vector((x, y, 0.34)),
                          Vector((x + 0.20, y + 0.03, 0.32))], 0.012, segs=6,
               taper=lambda t: 0.65 + 0.2 * (1.0 - t), tile=0.4, name="nestTwig{:02d}".format(i))

    # Wind-worn root fibres and broken branch stubs; keep their silhouette low and asymmetric.
    A.tube("wood", [Vector((-0.93, 0.22, 0.36)), Vector((-1.05, 0.42, 0.20)),
                     Vector((-1.22, 0.47, 0.04))], 0.075, segs=9,
           taper=lambda t: 1.0 - 0.72 * t, tile=0.9, name="rootEast")
    A.tube("wood", [Vector((0.34, -0.27, 0.94)), Vector((0.48, -0.34, 1.10)),
                     Vector((0.62, -0.40, 1.22))], 0.085, segs=9,
           taper=lambda t: 1.0 - 0.83 * t, tile=0.9, name="brokenBranch")
    A.tube("wood", [Vector((0.92, 0.28, 0.92)), Vector((1.08, 0.39, 1.02)),
                     Vector((1.25, 0.43, 1.00))], 0.060, segs=8,
           taper=lambda t: 1.0 - 0.78 * t, tile=0.9, name="rearBrokenRoot")
    # One off-centre splinter continues the torn edge into the bark.
    A.tube("wood", [Vector((-1.26, 0.25, 1.25)), Vector((-1.32, 0.18, 1.18)),
                     Vector((-1.35, 0.13, 1.12))], 0.038, segs=7,
           taper=lambda t: 1.0 - 0.88 * t, tile=0.8, name="entranceSplinter")

    # Moss sits in separate soft clumps so it remains readable rather than painting the whole log green.
    for i, (x, y, z, radii) in enumerate((
        (-0.78, -0.10, 1.19, (0.26, 0.17, 0.055)),
        (-0.30, 0.13, 1.23, (0.32, 0.14, 0.05)),
        (0.22, 0.08, 1.20, (0.29, 0.18, 0.060)),
        (0.78, -0.07, 1.15, (0.22, 0.13, 0.045)),
        (-0.54, 0.42, 0.97, (0.18, 0.13, 0.045)),
    )):
        A.rock("moss", radii, loc=(x, y, z), seed=43 + i, sub=2, rough=0.22,
               flat_bottom=0.35, tile=0.4, name="moss{:02d}".format(i))

    # Five simple collision shapes follow the shell. Keep the two low runners at
    # the sides: a full-width floor box here makes an impassable raised sill for
    # the rat capsule when the log is snapped to ground.
    A.collide_box((2.36, 0.12, 0.12), loc=(0, -0.30, 0.06))
    A.collide_box((2.36, 0.12, 0.12), loc=(0, 0.30, 0.06))
    A.collide_box((2.36, 0.20, 0.70), loc=(0, 0.50, 0.66))
    A.collide_box((2.36, 0.20, 0.70), loc=(0, -0.50, 0.66))
    A.collide_box((2.34, 0.72, 0.18), loc=(0, 0, 1.17))


def _triangulate_export_meshes(A):
    for obj, _material in A.parts:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        ngons = [face for face in bm.faces if len(face.verts) > 4]
        if ngons:
            bmesh.ops.triangulate(bm, faces=ngons, quad_method="BEAUTY", ngon_method="BEAUTY")
        bm.to_mesh(obj.data)
        bm.free()


def build(A):
    _hollow_log(A)
    _triangulate_export_meshes(A)
