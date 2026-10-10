"""Overshot water wheel on two stone piers, fed by a wooden flume. Axle along Y, wheel plane is XZ."""
import math

import lib

NAME = "WaterWheel"
BAKE_SIZE = 2048


def build(A):
    A.moss = (0.9, 0.5)
    HUB = (0.0, 0.0, 1.35)
    R = 1.1
    # axle, rims, spokes, radial paddles
    A.cyl("wood", 0.1, 1.75, loc=HUB, rot=(90, 0, 0), segs=20, name="axle")
    for y in (-0.5, 0.5):
        A.torus("wood", R, 0.05, loc=(0, y, HUB[2]), rot=(90, 0, 0), segs=48, rsegs=8, name="rim")
        A.torus("wood", R - 0.3, 0.03, loc=(0, y, HUB[2]), rot=(90, 0, 0), segs=40, rsegs=6, name="innerrim")
        for k in range(8):
            ang = k * 45.0
            d = (math.sin(math.radians(ang)), 0, math.cos(math.radians(ang)))
            c = (HUB[0] + d[0] * 0.55, y, HUB[2] + d[2] * 0.55)
            A.box("wood", (0.08, 0.06, 1.0), loc=c, rot=(0, ang, 0), bevel=0.004, name="spoke")
    n = 16
    for k in range(n):
        ang = k * 360.0 / n
        d = (math.sin(math.radians(ang)), 0, math.cos(math.radians(ang)))
        # back board (radial) and floor board (tangential) make a bucket
        A.box("wood", (0.035, 1.0, 0.26), loc=(d[0] * (R + 0.08), 0, HUB[2] + d[2] * (R + 0.08)), rot=(0, ang, 0), grain="Y", name="paddle")
        t = ang + 90.0 - 40.0
        tx, tz = math.sin(math.radians(ang + 14)), math.cos(math.radians(ang + 14))
        A.box("wood", (0.22, 1.0, 0.03), loc=(tx * (R - 0.02), 0, HUB[2] + tz * (R - 0.02)), rot=(0, ang + 70, 0), grain="Y", name="floor")
    # piers
    for y in (-0.82, 0.82):
        A.box("stone", (0.6, 0.4, 1.15), loc=(0, y, 0.575), bevel=0.02, tile=1.2, name="pier")
        A.box("stone", (0.7, 0.5, 0.1), loc=(0, y, 1.2), bevel=0.02, tile=1.2, name="pierCap")
        A.box("iron", (0.22, 0.06, 0.22), loc=(0, y * 0.9, HUB[2]), bevel=0.004, name="bearing")
    # flume feeding the top of the wheel
    A.box("wood", (1.5, 0.7, 0.05), loc=(-1.25, 0, 2.5), rot=(0, 6, 0), grain="X", bevel=0.004, name="flumeFloor")
    for y in (-0.36, 0.36):
        A.box("wood", (1.5, 0.05, 0.22), loc=(-1.25, y, 2.6), rot=(0, 6, 0), grain="X", bevel=0.004, name="flumeSide")
    A.box("water", (1.35, 0.6, 0.04), loc=(-1.25, 0, 2.55), rot=(0, 6, 0), name="flumeWater")
    for y in (-0.36, 0.36):
        for x in (-1.85, -0.85):
            A.box("wood", (0.1, 0.1, 2.45), loc=(x, y, 1.225), bevel=0.005, name="flumeLeg")
    A.box("wood", (1.1, 0.07, 0.07), loc=(-1.35, 0, 1.0), rot=(0, 0, 90), grain="Y", name="brace")
    A.collide_box((0.7, 0.5, 1.25), loc=(0, -0.82, 0.62))
    A.collide_box((0.7, 0.5, 1.25), loc=(0, 0.82, 0.62))
    A.collide_cyl(1.15, 1.1, loc=HUB, rot=(90, 0, 0), segs=16)
