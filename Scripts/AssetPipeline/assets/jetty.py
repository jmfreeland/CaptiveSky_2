"""Short timber jetty: plank deck on stringers, round piles with mooring posts at the end, rope coil, cleats, rope fenders."""
import math

from mathutils import Vector

NAME = "Jetty"


def build(A):
    rng = A.rng
    L, W, DZ = 4.2, 1.4, 0.86
    n = 28
    pw = L / n
    for i in range(n):  # deck planks across the width, slightly uneven
        x = -L / 2 + pw * (i + 0.5)
        A.box("wood", (pw - 0.012, W + rng.uniform(-0.03, 0.03), 0.055), loc=(x, rng.uniform(-0.01, 0.01), DZ),
              rot=(0, 0, rng.uniform(-0.6, 0.6)), grain="Y", bevel=0.004, name="plank")
    for y in (-0.55, 0.55):  # stringers + a rub rail along each edge
        A.box("wood", (L, 0.1, 0.16), loc=(0, y, DZ - 0.105), grain="X", bevel=0.005, name="stringer")
        A.box("wood", (L, 0.07, 0.09), loc=(0, y * 1.28, DZ + 0.03), grain="X", bevel=0.004, name="rail")
    for x in (-1.9, -0.65, 0.6, 1.85):  # piles, taller at the free end as mooring posts
        for y in (-0.62, 0.62):
            tall = x > 1.5
            h = 1.5 if tall else 1.0
            A.cyl("wood", 0.085, h, loc=(x, y, h / 2 - 0.02), segs=12, name="pile")
            if tall:
                A.torus("rope", 0.088, 0.012, loc=(x, y, 1.2), segs=14, rsegs=5, name="whipping")
    for x in (-1.2, 0.0, 1.2):  # cross braces under the deck
        A.box("wood", (0.07, 1.3, 0.07), loc=(x, 0, DZ - 0.24), grain="Y", name="crossbrace")
    # ladder rungs down the side
    for z in (0.25, 0.5, 0.72):
        A.box("wood", (0.08, 0.5, 0.05), loc=(-1.3, 0.66, z), name="rung")
    # rope coil, cleats, fenders
    for k in range(4):
        A.torus("rope", 0.17 - 0.012 * k, 0.026, loc=(1.15, 0.38, DZ + 0.06 + 0.04 * k), segs=24, rsegs=6, name="coil")
    for x in (-0.3, 0.9):
        A.box("wood", (0.28, 0.06, 0.05), loc=(x, -0.64, DZ + 0.075), name="cleat")
        A.box("wood", (0.06, 0.06, 0.06), loc=(x - 0.1, -0.64, DZ + 0.05), name="cleatFoot")
        A.box("wood", (0.06, 0.06, 0.06), loc=(x + 0.1, -0.64, DZ + 0.05), name="cleatFoot")
    for x in (-0.9, 0.4):
        A.cyl("rope", 0.075, 0.3, loc=(x, -0.74, DZ - 0.22), segs=14, uv="box", name="fender")
        A.cyl("rope", 0.008, 0.2, loc=(x, -0.7, DZ - 0.02), segs=5, uv="box", name="fenderRope")
    # mooring line trailing off the end post
    A.tube("rope", [Vector((1.85, 0.62, 1.1)), Vector((2.0, 0.8, 0.95)), Vector((2.25, 1.0, 0.8)), Vector((2.5, 1.15, 0.75))], 0.014, segs=6, name="line")
    A.collide_box((L, W, 0.9), loc=(0, 0, 0.45))
