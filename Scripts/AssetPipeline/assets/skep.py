"""Straw beehive skep on a little wooden stand."""
import math

NAME = "BeehiveSkep"


def build(A):
    # stand: platform + four splayed legs
    A.box("wood", (0.86, 0.86, 0.05), loc=(0, 0, 0.545), grain="X", bevel=0.006, name="plat")
    for sx in (-1, 1):
        for sy in (-1, 1):
            A.box("wood", (0.06, 0.06, 0.55), loc=(sx * 0.34, sy * 0.34, 0.26), rot=(-sy * 4, sx * 4, 0), bevel=0.004, name="leg")
    A.box("wood", (0.74, 0.04, 0.05), loc=(0, -0.34, 0.3), grain="X", name="brace")
    A.box("wood", (0.74, 0.04, 0.05), loc=(0, 0.34, 0.3), grain="X", name="brace")
    # skep: coiled dome, each coil is a slight bulge
    prof = [(0, 0.57)]
    H = 0.62
    coils = 13
    for i in range(coils + 1):
        z = H * (i / coils)
        base = 0.34 * math.sqrt(max(0.0, 1 - (z / H) ** 2.1)) + 0.0
        if i == coils:
            prof.append((0.0, 0.57 + H))
            break
        prof.append((base + 0.018, 0.57 + z + 0.012))
        prof.append((base + 0.004, 0.57 + z + 0.035))
    prof.append((0.0, 0.57 + H))
    A.lathe("strawcoil", prof, segs=40, tile=0.45, name="skep", sharp=60)
    # entrance and a tiny straw knot on top
    A.box("dirt", (0.11, 0.05, 0.065), loc=(0, -0.33, 0.63), name="door")
    A.rock("strawcoil", (0.045, 0.045, 0.05), loc=(0, 0, 1.2), seed=2, sub=1, tile=0.3, name="knot")
    # straw hackle: a loose cap tied over the top of the skep
    A.lathe("straw", [(0, 1.32), (0.05, 1.3), (0.2, 1.14), (0.31, 0.98), (0.33, 0.93), (0.29, 0.93), (0.2, 1.05), (0.08, 1.2), (0, 1.22)], segs=24, tile=0.5, name="hackle")
    A.torus("rope", 0.085, 0.012, loc=(0, 0, 1.2), segs=16, rsegs=5, name="hackleTie")
    A.collide_box((0.9, 0.9, 0.58), loc=(0, 0, 0.29))
    A.collide_cyl(0.36, 0.62, loc=(0, 0, 0.88), segs=14)
