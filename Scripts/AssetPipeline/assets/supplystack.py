"""Supply stack: stacked slatted crates, canvas sacks tied at the neck, a coil of rope, a nailed shipping tag."""
import math

from mathutils import Vector

NAME = "SupplyStack"


def crate(A, x, y, z, rot, sx=0.58, sy=0.42, sz=0.4):
    A.box("wood", (sx, sy, sz), loc=(x, y, z), rot=(0, 0, rot), grain="X", bevel=0.004, name="crate")
    c, s = math.cos(math.radians(rot)), math.sin(math.radians(rot))
    for dx in (-1, 1):  # corner battens (slightly proud) so it reads as a slatted crate
        for dy in (-1, 1):
            ox, oy = dx * (sx / 2 - 0.02), dy * (sy / 2 - 0.02)
            A.box("wood", (0.045, 0.045, sz + 0.012), loc=(x + ox * c - oy * s, y + ox * s + oy * c, z), rot=(0, 0, rot), bevel=0.003, name="batten")
    for dz in (-0.1, 0.1):  # horizontal slat lines
        A.box("wood", (sx + 0.012, sy + 0.012, 0.02), loc=(x, y, z + dz), rot=(0, 0, rot), grain="X", name="slat")


def sack(A, x, y, z, rot=0, k=1.0, seed=1):
    A.rock("canvas", (0.24 * k, 0.2 * k, 0.3 * k), loc=(x, y, z + 0.27 * k), rot=(0, 0, rot), seed=seed, sub=2, rough=0.09, flat_bottom=0.55, tile=0.35, name="sack")
    A.torus("rope", 0.095 * k, 0.014, loc=(x, y, z + 0.5 * k), segs=16, rsegs=5, name="tie")
    A.rock("canvas", (0.07 * k, 0.065 * k, 0.05 * k), loc=(x, y, z + 0.57 * k), seed=seed + 3, sub=1, rough=0.1, flat_bottom=0.3, tile=0.2, name="sackTop")


def build(A):
    crate(A, 0.0, 0.0, 0.2, 3)
    crate(A, 0.62, 0.04, 0.2, -6)
    crate(A, 0.08, 0.0, 0.61, 14, sx=0.52, sy=0.4, sz=0.38)
    # shipping tag nailed to a crate
    A.box("parchment", (0.12, 0.004, 0.08), loc=(0.62, -0.222, 0.22), rot=(0, 0, -6), name="tag")
    A.cyl("iron", 0.006, 0.01, loc=(0.575, -0.226, 0.25), rot=(90, 0, 0), segs=6, uv="box", name="nail")
    # sacks
    sack(A, -0.62, 0.05, 0.0, rot=20, seed=2)
    sack(A, -0.5, -0.42, 0.0, rot=-30, k=0.9, seed=6)
    sack(A, -0.88, -0.28, 0.0, rot=60, k=0.85, seed=9)
    A.rock("canvas", (0.24, 0.2, 0.3), loc=(0.55, 0.5, 0.2), rot=(0, 35, 10), seed=11, sub=2, rough=0.09, flat_bottom=0.5, tile=0.35, name="sackLean")
    A.torus("rope", 0.09, 0.014, loc=(0.52, 0.5, 0.34), rot=(0, 35, 0), segs=16, rsegs=5, name="tie")
    # rope coil on top and a short loose end
    for k in range(4):
        A.torus("rope", 0.15 - 0.01 * k, 0.024, loc=(0.1, 0.0, 0.8 + 0.03 * k), segs=24, rsegs=6, name="coil")
    A.tube("rope", [Vector((0.25, -0.05, 0.82)), Vector((0.4, -0.2, 0.8)), Vector((0.46, -0.28, 0.62)), Vector((0.5, -0.3, 0.3)), Vector((0.56, -0.38, 0.05)), Vector((0.78, -0.5, 0.025)), Vector((0.95, -0.46, 0.025))], 0.016, segs=6, name="loose")
    A.collide_box((1.9, 1.1, 0.85), loc=(-0.1, 0.0, 0.42))
