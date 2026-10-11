"""A small hearth loaf, cut slice, flour marks, linen and a well-used board."""
import math

from mathutils import Vector
import lib

NAME = "BreadPracticeSet"
BAKE_SIZE = 1024

# Isolated to this asset-builder process; the shared toolkit stays generic.
lib.TILED["bread"] = ("T_Lib_Bread", 0.30)
lib.PLAIN["bread_crumb"] = dict(BaseColor=[0.58, 0.38, 0.22], Roughness=0.94)
lib.PLAIN["bread_flour"] = dict(BaseColor=[0.78, 0.68, 0.52], Roughness=0.97)


def build(A):
    # A compact board with a warm, lightly worn working surface.
    A.box("wood", (0.54, 0.36, 0.025), loc=(0, 0, 0.0125), grain="X",
          bevel=0.016, name="bread_board")

    # An oval sourdough boule with its own small, tileable baked-crust material.
    loaf_center = (-0.045, 0.035, 0.102)
    loaf_radii = (0.13, 0.095, 0.075)
    A.rock("bread", loaf_radii, loc=loaf_center, rot=(2, -1, 7), seed=37,
           sub=2, rough=0.08, flat_bottom=0.96, tile=0.30, name="hearth_loaf")

    # Three shallow, arcing flour-scored seams following the loaf's upper dome.
    for index, lateral in enumerate((-0.026, 0.0, 0.026)):
        path = []
        for step in range(9):
            t = step / 8.0
            x = loaf_center[0] + (t - 0.5) * 0.19
            y = loaf_center[1] + lateral + 0.012 * math.sin(t * math.pi)
            nx = (x - loaf_center[0]) / loaf_radii[0]
            ny = (y - loaf_center[1]) / loaf_radii[1]
            dome = math.sqrt(max(0.025, 1.0 - nx * nx - ny * ny))
            z = loaf_center[2] + loaf_radii[2] * dome + 0.0015
            path.append(Vector((x, y, z)))
        A.tube("bread_flour", path, 0.0012, segs=5, name="flour_score_{}".format(index))

    # A cut face turned toward the viewer, with a thin darker crust around it.
    slice_center = (0.172, -0.071, 0.078)
    A.cyl("bread_crumb", 0.058, 0.026, loc=slice_center, rot=(90, 0, 0),
          segs=32, uv="box", name="cut_crumb")
    A.torus("bread", 0.052, 0.006, loc=(slice_center[0], slice_center[1] - 0.015,
                                        slice_center[2]), rot=(90, 0, 0),
            segs=32, rsegs=6, name="slice_crust")

    # A folded linen corner and a few crumbs make the set feel recently used.
    cloth_rows = []
    for row in range(3):
        y = -0.16 + row * 0.042
        cloth_row = []
        for col in range(4):
            x = -0.245 + col * 0.035
            fold = 0.008 if col == 2 and row == 1 else 0.002 * ((row + col) % 2)
            cloth_row.append(Vector((x, y, 0.030 + fold)))
        cloth_rows.append(cloth_row)
    A.cloth("canvas", cloth_rows, thick=0.003, name="folded_linen")

    for index, (x, y, z, radius) in enumerate((
        (0.236, -0.033, 0.029, 0.006),
        (0.222, -0.055, 0.028, 0.0045),
        (0.248, -0.069, 0.028, 0.0035),
    )):
        A.rock("mushstem", (radius, radius * 0.8, radius * 0.55),
               loc=(x, y, z), seed=81 + index, sub=1, rough=0.02,
               flat_bottom=0.6, name="crumb_{}".format(index))

    # Loose food stays non-blocking; only the board itself gets simple collision.
    A.collide_box((0.54, 0.36, 0.025), loc=(0, 0, 0.0125))
