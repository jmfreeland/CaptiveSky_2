"""Hay bales: one square bale with twine bands, two rolled bales lying on their sides."""
import math

from mathutils import Vector

NAME = "HayBales"


def build(A):
    A.box("straw", (0.9, 0.46, 0.46), loc=(0, 0, 0.23), grain="X", bevel=0.02, tile=0.5, name="square", smooth=True)
    for x in (-0.22, 0.22):  # four thin twine strips per band
        A.box("rope", (0.018, 0.482, 0.012), loc=(x, 0, 0.462), name="band_t")
        A.box("rope", (0.018, 0.482, 0.012), loc=(x, 0, 0.0), name="band_b")
        A.box("rope", (0.018, 0.012, 0.482), loc=(x, 0.234, 0.23), name="band_s")
        A.box("rope", (0.018, 0.012, 0.482), loc=(x, -0.234, 0.23), name="band_s")
    for (x, y, rot) in ((0.95, 0.25, 8), (-0.85, -0.55, -20)):  # rolled bales, axis along Y
        A.cyl("straw", 0.5, 0.9, loc=(x, y, 0.5), rot=(90, 0, rot), segs=28, tile=0.5, name="roll")
        for off in (-0.25, 0.25):
            pts = [Vector((0.508 * math.cos(a), 0.0, 0.508 * math.sin(a))) for a in [i * 2 * math.pi / 30 for i in range(32)]]
            ox = off * math.sin(math.radians(rot))
            oy = off * math.cos(math.radians(rot))
            A.tube("rope", pts, 0.013, segs=6, loc=(x - ox, y + oy, 0.5), rot=(0, 0, rot), name="twine")
    A.rock("straw", (0.25, 0.18, 0.06), loc=(0.35, -0.7, 0.04), seed=9, tile=0.4, name="pile")
    A.collide_box((0.95, 0.5, 0.48), loc=(0, 0, 0.24))
    A.collide_cyl(0.5, 0.9, loc=(0.95, 0.25, 0.5), rot=(90, 0, 8), segs=14)
    A.collide_cyl(0.5, 0.9, loc=(-0.85, -0.55, 0.5), rot=(90, 0, -20), segs=14)
