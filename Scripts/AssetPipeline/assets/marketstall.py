"""Market stall: four posts, counter, striped canvas awning with a scalloped valance, crates of apples, hanging sign."""
import math

from mathutils import Vector

NAME = "MarketStall"


def build(A):
    # posts and rails
    for sx in (-0.95, 0.95):
        A.box("wood", (0.09, 0.09, 2.3), loc=(sx, -0.55, 1.15), bevel=0.006, name="postFront")
        A.box("wood", (0.09, 0.09, 2.55), loc=(sx, 0.55, 1.275), bevel=0.006, name="postBack")
    A.box("wood", (2.0, 0.07, 0.08), loc=(0, 0.55, 2.3), grain="X", name="railBack")
    A.box("wood", (2.0, 0.07, 0.08), loc=(0, -0.55, 2.05), grain="X", name="railFront")
    # counter with front boards and a lower shelf
    A.box("wood", (2.0, 0.8, 0.06), loc=(0, -0.55, 0.92), grain="X", bevel=0.005, name="counter")
    A.box("wood", (1.96, 0.04, 0.85), loc=(0, -0.93, 0.47), grain="X", name="front")
    A.box("wood", (1.9, 0.6, 0.04), loc=(0, -0.55, 0.3), grain="X", name="shelf")
    # striped awning: alternating canvas / red strips, sagging between back rail and front rail
    n = 10
    w = 2.1 / n
    for i in range(n):
        x0, x1 = -1.05 + i * w, -1.05 + (i + 1) * w
        pts = []
        for xx in (x0, x1):
            row = []
            for t in [k / 7 for k in range(8)]:
                y = 0.62 - 1.35 * t
                z = 2.52 - 0.5 * t - 0.035 * math.sin(t * math.pi)
                row.append(Vector((xx, y, z)))
            pts.append(row)
        A.cloth("awning" if i % 2 == 0 else "canvas", pts, thick=0.008, name="awning")
        # scalloped valance hanging from the front edge
        zc = 2.0 - 0.5 * 0
        A.box("awning" if i % 2 == 0 else "canvas", (w - 0.004, 0.012, 0.16), loc=((x0 + x1) / 2, -0.73, 1.94), name="valance")
        A.cyl("awning" if i % 2 == 0 else "canvas", (w - 0.004) / 2, 0.012, loc=((x0 + x1) / 2, -0.73, 1.86), rot=(90, 0, 0), segs=14, uv="box", name="scallop")
    # crates of fruit on the counter
    for k, (cx, cr) in enumerate(((-0.55, 4), (0.15, -6), (0.75, 8))):
        A.box("wood", (0.5, 0.38, 0.16), loc=(cx, -0.55, 1.03), rot=(0, 0, cr), grain="X", bevel=0.004, name="crate")
        for j in range(9):
            a = j * 2.4 + k
            px = cx + 0.15 * math.cos(a) * (0.4 + 0.1 * (j % 3))
            py = -0.55 + 0.11 * math.sin(a)
            A.rock("awning" if k != 1 else "pumpkin", (0.058, 0.058, 0.055), loc=(px, py, 1.14 + 0.02 * (j % 2)), seed=j + k * 5, sub=1, rough=0.05, flat_bottom=0.9, name="apple")
    # crate on the floor + hanging sign
    A.box("wood", (0.5, 0.4, 0.3), loc=(-0.6, -0.55, 0.19), rot=(0, 0, -8), grain="X", bevel=0.004, name="floorCrate")
    A.box("wood", (0.5, 0.025, 0.28), loc=(0.55, -0.62, 1.78), grain="X", bevel=0.004, name="signboard")
    A.cyl("rope", 0.008, 0.25, loc=(0.4, -0.62, 1.93), segs=6, uv="box", name="signrope")
    A.cyl("rope", 0.008, 0.25, loc=(0.7, -0.62, 1.93), segs=6, uv="box", name="signrope")
    for i in range(4):
        A.box("parchment", (0.09, 0.004, 0.09), loc=(0.40 + i * 0.07, -0.636, 1.78), rot=(0, 3 * i, 0), name="price")
    A.collide_box((2.0, 1.2, 1.0), loc=(0, -0.45, 0.5))
    for sx in (-0.95, 0.95):
        A.collide_box((0.12, 0.12, 2.5), loc=(sx, 0.55, 1.25))
