"""Plank wheelbarrow with an iron-shod wheel; long axis along X, wheel at the front (+X)."""
import math

NAME = "Wheelbarrow"


def build(A):
    # frame: two rails run from the grips (rear, raised) to the wheel forks (front)
    tilt = 5.0
    cx, cz, L = -0.275, 0.5, 1.85
    for s in (1, -1):
        A.box("wood", (L, 0.045, 0.055), loc=(cx, s * 0.16, cz), rot=(0, tilt, 0), grain="X", bevel=0.004, name="rail")
        # grip: continues the rail past the rear end
        rear = cx - (L / 2) * math.cos(math.radians(tilt))
        rz = cz + (L / 2) * math.sin(math.radians(tilt))
        A.cyl("wood", 0.024, 0.22, loc=(rear - 0.09, s * 0.16, rz), rot=(0, 90 - tilt, 0), segs=10, name="grip")
        # front fork down to the axle
        A.box("wood", (0.04, 0.04, 0.3), loc=(0.62, s * 0.16, 0.31), bevel=0.003, name="fork")
        # rear leg from the rail to the ground, splayed a little
        A.box("wood", (0.045, 0.045, 0.52), loc=(-0.62, s * 0.19, 0.26), rot=(s * 5, -8, 0), bevel=0.003, name="leg")
    # tub sits on the rails
    t = 0.045
    A.box("wood", (0.74, 0.42, 0.035), loc=(0.0, 0, 0.55 + t), grain="X", bevel=0.004, name="floor")
    for s in (1, -1):
        A.box("wood", (0.8, 0.035, 0.28), loc=(0.0, s * 0.255, 0.68 + t), rot=(s * 18, 0, 0), grain="X", bevel=0.004, name="side")
    A.box("wood", (0.035, 0.52, 0.26), loc=(0.405, 0, 0.67 + t), rot=(0, 14, 0), grain="Y", bevel=0.004, name="front")
    A.box("wood", (0.035, 0.52, 0.22), loc=(-0.405, 0, 0.65 + t), rot=(0, -10, 0), grain="Y", bevel=0.004, name="back")
    # wheel: axis along Y, iron tyre + hub between the forks
    wc = (0.62, 0, 0.2)
    A.torus("wood", 0.172, 0.022, loc=wc, rot=(90, 0, 0), segs=28, rsegs=6, name="wheelRim")
    A.torus("iron", 0.196, 0.012, loc=wc, rot=(90, 0, 0), segs=28, rsegs=5, name="tyre")
    A.cyl("wood", 0.045, 0.1, loc=wc, rot=(90, 0, 0), segs=12, uv="box", name="hub")
    for k in range(8):  # spokes
        ang = k * 45.0
        d = (math.sin(math.radians(ang)), 0, math.cos(math.radians(ang)))
        A.box("wood", (0.028, 0.028, 0.16), loc=(wc[0] + d[0] * 0.105, 0, wc[2] + d[2] * 0.105), rot=(0, ang, 0), name="spoke")
    A.cyl("wood", 0.035, 0.38, loc=(0.62, 0, 0.2), rot=(90, 0, 0), segs=12, uv="box", name="axle")
    A.box("wood", (0.04, 0.4, 0.04), loc=(-0.6, 0, 0.44), grain="Y", name="spreader")
    # a load of straw in the tub
    A.rock("straw", (0.24, 0.15, 0.1), loc=(0.0, 0, 0.66 + t), seed=5, tile=0.4, name="load")
    A.collide_box((1.9, 0.6, 0.9), loc=(-0.25, 0, 0.45))
