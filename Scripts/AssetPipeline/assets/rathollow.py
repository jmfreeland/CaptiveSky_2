"""A weathered fallen trunk hollow with a rat-sized entrance and a sheltered nest bed."""
import math

import bmesh
from mathutils import Vector
import lib


NAME = "RatHollow_BarkV4"
ENTRANCE_BARK_BREAK_SEGMENTS = tuple(range(7, 14))
ENTRANCE_RIM_IRREGULARITY = 1.65
lib.TILED["bark"] = ("T_Lib_RatHollow_Bark", 1.2)


def _ring(bm, x, radius, center_z, phase, segments, center_y=0.0, irregularity=1.0):
    verts = []
    for i in range(segments):
        angle = 2.0 * math.pi * i / segments
        # A few broad, offset deformations keep the trunk from reading as a
        # manufactured cylinder; the small high-frequency term breaks the rim.
        variation = (1.0 + irregularity * (
            0.085 * math.sin(3.0 * angle + phase)
            + 0.038 * math.sin(7.0 * angle - phase * 0.7)
            + 0.032 * math.sin(2.0 * angle + phase * 1.3)))
        x_jitter = 0.045 * math.sin(5.0 * angle + phase)
        r = radius * variation
        verts.append(bm.verts.new((x + x_jitter, center_y + r * math.cos(angle), center_z + r * math.sin(angle))))
    return verts


def _outer_surface_point(x, angle, stations):
    upper = next((index for index, station in enumerate(stations) if station[0] >= x), len(stations) - 1)
    lower = max(0, upper - 1)
    first, second = stations[lower], stations[upper]
    amount = 0.0 if upper == lower else (x - first[0]) / (second[0] - first[0])
    radius = first[1] + (second[1] - first[1]) * amount
    center_z = first[2] + (second[2] - first[2]) * amount
    first_y = 0.035 * math.sin(lower * 0.83)
    second_y = 0.035 * math.sin(upper * 0.83)
    center_y = first_y + (second_y - first_y) * amount
    phase = (lower + amount) * 0.61
    variation = 1.0 + (
        0.085 * math.sin(3.0 * angle + phase)
        + 0.038 * math.sin(7.0 * angle - phase * 0.7)
        + 0.032 * math.sin(2.0 * angle + phase * 1.3)
    )
    radius *= variation
    return Vector((
        x + 0.045 * math.sin(5.0 * angle + phase),
        center_y + radius * math.cos(angle),
        center_z + radius * math.sin(angle),
    ))


def _moss_patch_grid(center_x, center_angle, length, angle_width, seed, stations):
    rows = []
    row_count, column_count = 7, 5
    for row in range(row_count):
        u = row / (row_count - 1) * 2.0 - 1.0
        points = []
        for column in range(column_count):
            v = column / (column_count - 1) * 2.0 - 1.0
            x_edge = 0.88 + 0.12 * math.sin(seed + column * 1.7 + row * 0.43)
            angle_edge = 0.88 + 0.12 * math.sin(seed * 0.7 + row * 1.3 - column * 0.51)
            x = center_x + u * (length * 0.5) * x_edge
            angle = center_angle + v * (angle_width * 0.5) * angle_edge
            point = _outer_surface_point(x, angle, stations)
            interior = max(0.0, 1.0 - u * u) * max(0.0, 1.0 - v * v)
            point += Vector((0.0, math.cos(angle), math.sin(angle))) * (0.006 + 0.006 * interior)
            points.append(point)
        rows.append(points)
    return rows


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


def _hollow_log(A, conform_moss=False):
    segments = 40
    center_z = 0.66
    bm = bmesh.new()

    outer_stations = [
        (-1.30, 0.56, 0.65), (-1.17, 0.64, 0.66), (-0.88, 0.68, 0.69),
        (-0.48, 0.64, 0.65), (0.05, 0.66, 0.68), (0.55, 0.60, 0.64),
        (1.00, 0.59, 0.66), (1.24, 0.50, 0.64),
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

    A._finish(bm, "hollowBark", "bark", (0, 0, 0), (0, 0, 0), "box", "X", 1.2, True, 55)

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
    A.tube("bark", [Vector((-0.93, 0.22, 0.36)), Vector((-1.05, 0.42, 0.20)),
                     Vector((-1.22, 0.47, 0.04))], 0.075, segs=9,
           taper=lambda t: 1.0 - 0.72 * t, tile=0.9, name="rootEast")
    A.tube("bark", [Vector((0.34, -0.27, 0.94)), Vector((0.48, -0.34, 1.10)),
                     Vector((0.62, -0.40, 1.22))], 0.085, segs=9,
           taper=lambda t: 1.0 - 0.83 * t, tile=0.9, name="brokenBranch")
    A.tube("bark", [Vector((0.92, 0.28, 0.92)), Vector((1.08, 0.39, 1.02)),
                     Vector((1.25, 0.43, 1.00))], 0.060, segs=8,
           taper=lambda t: 1.0 - 0.78 * t, tile=0.9, name="rearBrokenRoot")
    # One off-centre splinter continues the torn edge into the bark.
    A.tube("bark", [Vector((-1.26, 0.25, 1.25)), Vector((-1.32, 0.18, 1.18)),
                     Vector((-1.35, 0.13, 1.12))], 0.038, segs=7,
           taper=lambda t: 1.0 - 0.88 * t, tile=0.8, name="entranceSplinter")
    A.tube("bark", [Vector((-1.20, -0.30, 0.98)), Vector((-1.30, -0.43, 1.07)),
                     Vector((-1.42, -0.52, 0.98))], 0.060, segs=8,
           taper=lambda t: 1.0 - 0.86 * t, tile=0.8, name="splitRimEast")
    A.tube("bark", [Vector((-1.12, 0.34, 0.92)), Vector((-1.25, 0.48, 0.99)),
                     Vector((-1.36, 0.55, 0.87))], 0.052, segs=8,
           taper=lambda t: 1.0 - 0.84 * t, tile=0.8, name="splitRimWest")

    moss_sites = (
        (-0.78, 1.58, 0.48, 0.44),
        (-0.30, 1.48, 0.58, 0.48),
        (0.22, 1.64, 0.50, 0.52),
        (0.78, 1.76, 0.38, 0.43),
        (-0.54, 1.10, 0.34, 0.48),
    )
    if conform_moss:
        for i, (x, angle, length, angle_width) in enumerate(moss_sites):
            A.cloth(
                "moss",
                _moss_patch_grid(x, angle, length, angle_width, 43 + i, outer_stations),
                thick=0.006,
                tile=0.4,
                name="mossPatch{:02d}".format(i),
            )
    else:
        moss_rocks = (
            (-0.78, -0.10, 1.30, (0.28, 0.18, 0.035)),
            (-0.30, 0.13, 1.31, (0.34, 0.16, 0.038)),
            (0.22, 0.08, 1.30, (0.31, 0.19, 0.035)),
            (0.78, -0.07, 1.24, (0.24, 0.15, 0.030)),
            (-0.54, 0.42, 1.17, (0.20, 0.15, 0.030)),
        )
        for i, (x, y, z, radii) in enumerate(moss_rocks):
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
