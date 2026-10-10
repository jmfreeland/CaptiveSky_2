"""Scarecrow on a pole: stuffed sack head with a stitched face, patched blue shirt, felt hat, straw tufts,
and a small crow perched on one arm (a nod to the island's Raven)."""
import math

import lib

NAME = "Scarecrow"


def build(A):
    rng = A.rng
    # pole, crossbar, ground stakes
    A.cyl("wood", 0.045, 2.1, loc=(0, 0, 1.05), segs=12, name="pole")
    A.cyl("wood", 0.035, 1.6, loc=(0, 0, 1.55), rot=(0, 90, 0), segs=12, name="arms")
    for sx in (-1, 1):
        A.box("wood", (0.05, 0.05, 0.5), loc=(sx * 0.17, 0.07, 0.2), rot=(0, sx * -22, 0), name="stake")
    # torso (shirt) + hem straw + belt
    A.rock("cloth_blue", (0.21, 0.15, 0.34), loc=(0, 0, 1.4), seed=3, sub=2, rough=0.05, flat_bottom=0.95, name="shirt")
    A.torus("rope", 0.19, 0.016, loc=(0, 0, 1.2), segs=24, rsegs=6, name="belt")
    A.box("canvas", (0.15, 0.012, 0.13), loc=(0.08, -0.145, 1.45), rot=(0, 0, 8), name="patch")
    A.box("awning", (0.1, 0.012, 0.1), loc=(-0.07, -0.14, 1.32), rot=(0, 0, -12), name="patch")
    # sleeves + straw cuffs
    for sx in (-1, 1):
        A.cyl("cloth_blue", 0.075, 0.62, loc=(sx * 0.48, 0, 1.56), rot=(0, 90, 0), segs=14, name="sleeve")
        for k in range(9):
            a = rng.uniform(0, 2 * math.pi)
            tip = (sx * (0.82 + rng.uniform(0, 0.08)), 0.03 * math.cos(a), 1.56 + 0.05 * math.sin(a))
            A.cyl("straw", 0.006, 0.17, loc=tip, rot=(0, 90 + sx * rng.uniform(-35, 35), rng.uniform(-25, 25)), segs=5, uv="box", name="straw")
    for k in range(12):  # hem + neck straw
        a = 2 * math.pi * k / 12
        A.cyl("straw", 0.006, 0.16, loc=(0.18 * math.cos(a), 0.12 * math.sin(a), 1.1), rot=(rng.uniform(-20, 20), rng.uniform(-20, 20), 0), segs=5, uv="box", name="hem")
    # head: sack with stitched face
    A.rock("canvas", (0.15, 0.14, 0.17), loc=(0, 0, 1.86), seed=7, sub=2, rough=0.05, flat_bottom=0.95, tile=0.3, name="head")
    for sx in (-0.055, 0.055):
        A.cyl("char", 0.018, 0.012, loc=(sx, -0.128, 1.9), rot=(90, 0, 0), segs=10, uv="box", name="eye")
    for k in range(7):  # stitched smile
        t = (k / 6 - 0.5) * 2
        A.box("char", (0.02, 0.008, 0.006), loc=(t * 0.07, -0.133, 1.82 - 0.018 * (1 - t * t)), rot=(0, -t * 25, 0), name="stitch")
    A.torus("rope", 0.085, 0.01, loc=(0, 0, 1.7), segs=20, rsegs=5, name="neckTie")
    # hat
    A.cyl("felt", 0.31, 0.025, loc=(0, 0, 1.99), segs=28, uv="box", name="brim")
    A.lathe("felt", [(0, 1.99), (0.16, 1.99), (0.15, 2.08), (0.1, 2.15), (0, 2.16)], segs=24, name="crown")
    A.torus("awning", 0.157, 0.012, loc=(0, 0, 2.02), segs=24, rsegs=5, name="hatBand")
    # crow on the right arm
    cx, cy, cz = 0.62, 0.0, 1.69
    A.rock("char", (0.045, 0.085, 0.05), loc=(cx, cy, cz + 0.05), rot=(0, 0, 90), seed=2, sub=2, rough=0.03, flat_bottom=0.9, name="crowBody")
    A.rock("char", (0.032, 0.032, 0.032), loc=(cx, cy - 0.075, cz + 0.1), seed=4, sub=1, rough=0.02, flat_bottom=0.9, name="crowHead")
    A.cyl("char", 0.011, 0.045, loc=(cx, cy - 0.122, cz + 0.098), r2=0.003, rot=(90, 0, 0), segs=8, uv="box", name="beak")
    A.box("char", (0.03, 0.09, 0.012), loc=(cx, cy + 0.1, cz + 0.045), rot=(15, 0, 0), name="tail")
    for sx in (-0.012, 0.012):
        A.cyl("iron", 0.003, 0.05, loc=(cx + sx, cy, cz + 0.015), segs=5, uv="box", name="leg")
    A.collide_box((1.7, 0.4, 2.2), loc=(0, 0, 1.1))
