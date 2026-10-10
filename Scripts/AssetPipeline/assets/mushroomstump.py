"""Old tree stump with root flares, moss clumps and a cluster of red spotted mushrooms."""
import math

from mathutils import Euler, Vector

import lib

NAME = "MushroomStump"


def mushroom(A, x, y, z, s, tilt=(0, 0), spots=6):
    rx, ry = tilt
    stem = [(0, 0), (0.032, 0), (0.026, 0.05), (0.022, 0.1), (0.03, 0.135), (0, 0.14)]
    cap = [(0, 0.19), (0.04, 0.188), (0.1, 0.165), (0.14, 0.11), (0.15, 0.06), (0.135, 0.05), (0.09, 0.062), (0.04, 0.085), (0, 0.09)]
    stem = [(r * s, h * s) for r, h in stem]
    cap = [(r * s, (h + 0.075) * s) for r, h in cap]
    A.lathe("mushstem", stem, loc=(x, y, z), rot=(rx, ry, 0), segs=14, name="stem")
    A.lathe("mushcap", cap, loc=(x, y, z), rot=(rx, ry, 0), segs=20, name="cap")
    rng = A.rng
    prof = [(0.0, 0.265), (0.04, 0.263), (0.1, 0.24), (0.14, 0.185), (0.15, 0.135)]  # cap dome height vs radius (unscaled)

    def dome(r):
        for (r0, h0), (r1, h1) in zip(prof, prof[1:]):
            if r <= r1:
                return h0 + (h1 - h0) * (r - r0) / max(r1 - r0, 1e-6)
        return prof[-1][1]

    for k in range(spots):
        a = rng.uniform(0, 2 * math.pi)
        rr = rng.uniform(0.02, 0.1)
        p = Vector((rr * math.cos(a) * s, rr * math.sin(a) * s, (dome(rr) + 0.004) * s))
        p.rotate(Euler((math.radians(rx), math.radians(ry), 0)))
        A.rock("mushstem", (0.026 * s, 0.026 * s, 0.014 * s), loc=(x + p.x, y + p.y, z + p.z), rot=(rx, ry, 0), seed=k, sub=1, rough=0.02, flat_bottom=0.9, name="spot")


def build(A):
    A.moss = (0.2, 0.5)
    rng = A.rng
    # stump body with flared base and a broken, slightly concave top
    prof = [(0, 0.0), (0.56, 0.0), (0.46, 0.07), (0.4, 0.2), (0.37, 0.4), (0.375, 0.52), (0.34, 0.56), (0.2, 0.545), (0, 0.54)]
    A.lathe("wood", prof, segs=28, tile=0.9, sharp=70, name="stump")
    # roots
    for k in range(5):
        a = math.radians(k * 72 + 10)
        pts = [Vector((0.3 * math.cos(a), 0.3 * math.sin(a), 0.14)), Vector((0.5 * math.cos(a + 0.1), 0.5 * math.sin(a + 0.1), 0.05)),
               Vector((0.72 * math.cos(a + 0.15), 0.72 * math.sin(a + 0.15), 0.0))]
        A.tube("wood", pts, 0.07, segs=8, taper=lambda t: 1.0 - 0.75 * t, tile=0.9, name="root")
    # moss clumps on top, shoulder and roots (tiled moss texture)
    for (x, y, z, rad) in ((0.05, 0.1, 0.55, (0.24, 0.2, 0.05)), (-0.2, -0.12, 0.54, (0.14, 0.12, 0.04)), (0.34, -0.12, 0.3, (0.1, 0.12, 0.1)),
                           (-0.35, 0.18, 0.18, (0.12, 0.1, 0.1)), (0.0, 0.62, 0.04, (0.2, 0.14, 0.06)), (-0.5, -0.4, 0.04, (0.16, 0.12, 0.05))):
        A.rock("moss", rad, loc=(x, y, z), seed=int(abs(x * 17 + y * 5)) + 3, sub=2, rough=0.22, flat_bottom=0.4, tile=0.4, name="moss")
    # mushrooms: cluster on the shoulder, a few on the ground
    mushroom(A, 0.30, -0.15, 0.50, 1.15, tilt=(-8, 14))
    mushroom(A, 0.12, -0.3, 0.46, 0.8, tilt=(-6, 4))
    mushroom(A, 0.37, 0.05, 0.42, 0.6, tilt=(5, 20))
    mushroom(A, -0.1, -0.02, 0.55, 0.7, tilt=(0, -10))
    mushroom(A, 0.55, -0.35, 0.0, 1.0, tilt=(0, 6))
    mushroom(A, 0.7, -0.15, 0.0, 0.6, tilt=(4, 10))
    mushroom(A, -0.45, 0.55, 0.0, 0.75, tilt=(-4, -8))
    A.collide_cyl(0.5, 0.56, loc=(0, 0, 0.28), segs=12)
