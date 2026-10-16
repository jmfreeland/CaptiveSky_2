"""A fallen, hollow woodland trunk for the RatHollow microhabitat."""

import math

import bmesh
import lib


NAME = "RatHollowRootArch"
lib.TILED["bark"] = ("T_Lib_RatHollow_Bark", 1.2)
lib.PLAIN["hollowwood"] = {"BaseColor": [0.22, 0.12, 0.065], "Roughness": 1.0}


def _trunk_shell():
    bm = bmesh.new()
    sides = 20
    rings = (
        (-0.78, -0.01, 0.47, 0.45, 0.43),
        (-0.63, -0.01, 0.47, 0.46, 0.44),
        (-0.38, 0.00, 0.46, 0.43, 0.42),
        (-0.08, 0.02, 0.44, 0.40, 0.39),
        (0.23, 0.04, 0.42, 0.38, 0.37),
        (0.52, 0.05, 0.40, 0.36, 0.35),
        (0.78, 0.04, 0.39, 0.33, 0.33),
    )
    outer = []
    for ring_index, (x, cy, cz, ry, rz) in enumerate(rings):
        row = []
        for side in range(sides):
            angle = side * math.tau / sides
            irregular = 1.0 + 0.045 * math.sin(3.0 * angle + ring_index * 0.71) \
                + 0.025 * math.cos(5.0 * angle - ring_index * 0.43)
            row.append(bm.verts.new((
                x + 0.018 * math.sin(angle * 2.0 + ring_index),
                cy + math.cos(angle) * ry * irregular,
                cz + math.sin(angle) * rz * irregular,
            )))
        outer.append(row)

    for previous, current in zip(outer, outer[1:]):
        for side in range(sides):
            next_side = (side + 1) % sides
            bm.faces.new((previous[side], previous[next_side], current[next_side], current[side]))

    # A ragged annular rim makes the mouth visibly open, rather than a capped
    # log end. The inner shell is a separate dark material behind this lip.
    front_inner = []
    for side in range(sides):
        angle = side * math.tau / sides
        irregular = 1.0 + 0.035 * math.sin(3.0 * angle + 0.4)
        front_inner.append(bm.verts.new((
            -0.78 + 0.012 * math.sin(angle * 2.0),
            -0.01 + math.cos(angle) * 0.275 * irregular,
            0.47 + math.sin(angle) * 0.275 * irregular,
        )))
    for side in range(sides):
        next_side = (side + 1) % sides
        bm.faces.new((outer[0][side], front_inner[side], front_inner[next_side], outer[0][next_side]))

    # The far end is broken and angled, but closed so the hollow has a deep,
    # sheltered back rather than daylight visible through the whole log.
    back_center = bm.verts.new((0.81, 0.035, 0.39))
    for side in range(sides):
        next_side = (side + 1) % sides
        bm.faces.new((outer[-1][side], outer[-1][next_side], back_center))
    return bm


def _interior_shell():
    bm = bmesh.new()
    sides = 20
    rings = (
        (-0.78, -0.01, 0.47, 0.275, 0.275),
        (-0.60, -0.01, 0.47, 0.265, 0.26),
        (-0.34, 0.00, 0.46, 0.245, 0.24),
        (-0.04, 0.02, 0.44, 0.225, 0.22),
        (0.25, 0.04, 0.42, 0.20, 0.20),
        (0.51, 0.05, 0.40, 0.16, 0.17),
        (0.67, 0.045, 0.395, 0.075, 0.085),
    )
    rows = []
    for ring_index, (x, cy, cz, ry, rz) in enumerate(rings):
        row = []
        for side in range(sides):
            angle = side * math.tau / sides
            irregular = 1.0 + 0.025 * math.sin(4.0 * angle + ring_index * 0.8)
            row.append(bm.verts.new((
                x,
                cy + math.cos(angle) * ry * irregular,
                cz + math.sin(angle) * rz * irregular,
            )))
        rows.append(row)

    for previous, current in zip(rows, rows[1:]):
        for side in range(sides):
            next_side = (side + 1) % sides
            bm.faces.new((previous[side], current[side], current[next_side], previous[next_side]))
    back_center = bm.verts.new((0.68, 0.045, 0.395))
    for side in range(sides):
        bm.faces.new((rows[-1][side], back_center, rows[-1][(side + 1) % sides]))
    return bm


def build(A):
    A._finish(_trunk_shell(), "outerShell", "bark", (0, 0, 0), (0, 0, 0), "box", "X", 1.2, True, 40)
    # Warm inner wood keeps the recess legible in daylight without the charred
    # surface turning the opening into a featureless black void.
    A._finish(_interior_shell(), "innerShell", "hollowwood", (0, 0, 0), (0, 0, 0), "box", "X", 0.72, True, 40)

    # The broken root plate is concentrated behind and to one side. It should
    # merge with the ground and hide the silhouette at human height in context.
    A.tube(
        "bark",
        [(0.53, 0.02, 0.27), (0.68, -0.18, 0.22), (0.91, -0.44, 0.11), (1.04, -0.56, 0.055)],
        0.13, segs=9, taper=lambda t: 1.0 - 0.66 * t, tile=0.82, grain="X", name="rootFanNear",
    )
    A.tube(
        "bark",
        [(0.59, 0.12, 0.24), (0.81, 0.32, 0.18), (1.00, 0.56, 0.09)],
        0.12, segs=9, taper=lambda t: 1.0 - 0.68 * t, tile=0.82, grain="X", name="rootFanFar",
    )
    A.tube(
        "bark",
        [(0.73, 0.02, 0.24), (0.89, 0.08, 0.39), (1.02, 0.14, 0.49)],
        0.105, segs=9, taper=lambda t: 1.0 - 0.64 * t, tile=0.78, grain="X", name="brokenRootFork",
    )

    # A few moss pads follow the trunk crown; this is deliberately a sparse
    # patch pattern rather than a floating blanket across the log.
    A.rock("moss", (0.27, 0.17, 0.028), loc=(-0.10, -0.05, 0.795),
           seed=11, sub=1, rough=0.06, flat_bottom=0.82, tile=0.30, name="mossPadNear")
    A.rock("moss", (0.20, 0.14, 0.025), loc=(0.37, 0.07, 0.755),
           seed=23, sub=1, rough=0.06, flat_bottom=0.82, tile=0.30, name="mossPadFar")

    # Shallow soil and a few nest twigs sit inside the tunnel, not across the
    # entrance. The placement record and navmesh remain the gameplay truth.
    A.rock("dirt", (0.19, 0.27, 0.09), loc=(0.50, 0.045, 0.095),
           seed=31, sub=1, rough=0.10, flat_bottom=0.88, tile=0.48, name="rearSoil")
    # A small bedding patch near the entrance makes the shelter read as lived
    # in from a three-quarter view while leaving the lower passage open.
    A.rock("straw", (0.15, 0.12, 0.022), loc=(-0.43, 0.0, 0.235),
           seed=47, sub=1, rough=0.06, flat_bottom=0.92, tile=0.4, name="frontNestBed")
    for index, (start, mid, end, radius) in enumerate((
        ((-0.56, -0.10, 0.23), (-0.39, -0.02, 0.26), (-0.22, 0.08, 0.24), 0.014),
        ((-0.54, 0.09, 0.23), (-0.37, 0.03, 0.255), (-0.20, -0.08, 0.24), 0.012),
        ((-0.51, -0.05, 0.225), (-0.34, 0.10, 0.25), (-0.22, 0.04, 0.235), 0.011),
        ((-0.52, 0.05, 0.225), (-0.36, -0.09, 0.25), (-0.22, -0.06, 0.235), 0.011),
    )):
        A.tube("straw", [start, mid, end], radius, segs=6,
               taper=lambda t: 0.65 + 0.25 * math.sin(math.pi * t), tile=0.4,
               name="nestTwig{:02d}".format(index))

    # Tube end caps are n-gons (six or nine sides); Blender's FBX exporter
    # warns that it cannot export tangent space for those faces. Triangulate
    # only those caps before export, leaving the UV-mapped shell quads intact.
    for obj, _material in A.parts:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        ngons = [face for face in bm.faces if len(face.verts) > 4]
        if ngons:
            bmesh.ops.triangulate(bm, faces=ngons, ngon_method="BEAUTY")
            bm.to_mesh(obj.data)
            obj.data.update()
        bm.free()
