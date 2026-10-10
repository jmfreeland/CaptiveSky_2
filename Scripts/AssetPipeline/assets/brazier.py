"""Iron fire brazier on three splayed legs with a glowing coal bed (emissive) and two ring handles."""
import math

from mathutils import Vector

import lib

NAME = "Brazier"


def build(A):
    # bowl, rim lip, outer ring
    bowl = [(0, 0.62), (0.16, 0.635), (0.3, 0.72), (0.36, 0.84), (0.375, 0.88), (0.345, 0.88), (0.325, 0.855),
            (0.26, 0.76), (0.14, 0.69), (0, 0.675)]
    A.lathe("iron", bowl, segs=36, smooth=True, sharp=50, name="bowl")
    A.torus("iron", 0.36, 0.014, loc=(0, 0, 0.875), segs=36, rsegs=6, name="lip")
    A.torus("iron", 0.205, 0.012, loc=(0, 0, 0.58), segs=28, rsegs=6, name="collar")
    # legs
    for k in range(3):
        a = math.radians(k * 120 + 30)
        top = Vector((0.2 * math.cos(a), 0.2 * math.sin(a), 0.64))
        foot = Vector((0.4 * math.cos(a), 0.4 * math.sin(a), 0.0))
        c, d = (top + foot) / 2, top - foot
        A.box("iron", (0.045, 0.03, d.length), loc=tuple(c), rot=lib.align(d), bevel=0.003, name="leg")
        A.cyl("iron", 0.03, 0.03, loc=tuple(foot + Vector((0, 0, 0.015))), segs=10, uv="box", name="foot")
    A.torus("iron", 0.285, 0.011, loc=(0, 0, 0.3), segs=32, rsegs=6, name="legRing")
    # handles
    for s in (-1, 1):
        A.torus("iron", 0.065, 0.011, loc=(s * 0.4, 0, 0.82), rot=(90, 0, 0), segs=16, rsegs=5, name="handle")
    # coals, ash and a half-burnt log
    A.cyl("ash", 0.3, 0.02, loc=(0, 0, 0.7), segs=20, uv="box", name="ashbed")
    rng = A.rng
    for k in range(18):  # a heaped bed of coals that reaches the rim so the glow reads from outside
        a = rng.uniform(0, 2 * math.pi)
        r = rng.uniform(0.0, 0.28)
        lift = 0.1 * (1 - r / 0.3)
        A.rock("coal", (0.075, 0.065, 0.05), loc=(r * math.cos(a), r * math.sin(a), 0.78 + lift + 0.012 * (k % 3)), seed=k + 40, sub=1, rough=0.22, flat_bottom=0.5, name="coal")
    A.cyl("char", 0.045, 0.34, loc=(0.0, 0.0, 0.9), rot=(0, 78, 25), segs=10, uv="box", name="log")
    A.cyl("coal", 0.047, 0.1, loc=(0.1, 0.045, 0.895), rot=(0, 78, 25), segs=10, uv="box", name="logGlow")
    A.collide_cyl(0.4, 0.9, loc=(0, 0, 0.45), segs=12)
