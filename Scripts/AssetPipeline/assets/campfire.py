"""Campfire ring: mossy stone circle, leaning charred logs, ash bed, glowing embers."""
import math

from mathutils import Euler, Vector

import lib

NAME = "CampfireRing"
BAKE_SIZE = 2048


def build(A):
    A.moss = (0.26, 0.58)  # small, drier stones: patchy moss on tops, not a green blanket
    n = 10
    for i in range(n):
        a = 2 * math.pi * i / n + (0.08 if i % 2 else 0)
        r = 0.58 + (0.02 if i % 2 else 0)
        A.rock("stone", (0.15 + 0.03 * (i % 3), 0.12 + 0.02 * (i % 2), 0.1 + 0.02 * (i % 3)),
               loc=(r * math.cos(a), r * math.sin(a), 0.075), rot=(0, 0, math.degrees(a) + 20 * i), seed=i + 2, tile=1.2, name="ring")
    A.cyl("ash", 0.46, 0.03, loc=(0, 0, 0.015), segs=28, uv="box", name="ash")
    # leaning logs
    yaws = [0, 75, 150, 215, 290]
    for k, yaw in enumerate(yaws):
        th = math.radians(58 + 4 * (k % 2))
        length = 0.78
        base_r = 0.34
        # centre of the log: base at radius base_r on its yaw side, top leaning toward the middle
        cy = base_r - 0.5 * length * math.sin(th)
        cz = 0.5 * length * math.cos(th) + 0.04
        y = math.radians(yaw)
        loc = (-cy * math.sin(y), cy * math.cos(y), cz)
        A.cyl("wood", 0.052, length, loc=loc, rot=(math.degrees(th), 0, yaw), segs=14, tile=0.5, name="log")
        # charred tip
        ax = Vector((0, -math.sin(th), math.cos(th)))
        ax.rotate(Euler((0, 0, y)))
        c = Vector(loc) + ax * (0.5 * length - 0.09)
        A.cyl("char", 0.056, 0.18, loc=tuple(c), rot=(math.degrees(th), 0, yaw), segs=14, uv="box", name="tip")
    for i in range(5):
        a = i * 1.3
        A.rock("ember", (0.07, 0.06, 0.04), loc=(0.12 * math.cos(a), 0.12 * math.sin(a), 0.045), seed=20 + i, sub=1, name="ember")
    # cooking tripod with a hanging iron pot
    top = Vector((0, 0, 1.3))
    for k in range(3):
        a = math.radians(k * 120 + 15)
        foot = Vector((0.95 * math.cos(a), 0.95 * math.sin(a), 0.0))
        A.box("wood", (0.06, 0.06, (top - foot).length), loc=tuple((top + foot) / 2), rot=lib.align(top - foot), bevel=0.004, name="tripodLeg")
    A.torus("rope", 0.05, 0.012, loc=(0, 0, 1.22), segs=14, rsegs=5, name="lashing")
    A.tube("iron", [Vector((0, 0, 1.25)), Vector((0, 0, 1.0)), Vector((0, 0, 0.82))], 0.008, segs=5, name="chain")
    A.lathe("iron", [(0, 0.55), (0.1, 0.55), (0.17, 0.6), (0.2, 0.7), (0.19, 0.78), (0.205, 0.795), (0.17, 0.795), (0.16, 0.77), (0.15, 0.62), (0, 0.6)], segs=24, name="pot")
    A.torus("iron", 0.15, 0.008, loc=(0, 0, 0.84), rot=(90, 0, 0), segs=16, rsegs=5, name="potHandle")
    A.collide_cyl(0.75, 0.2, loc=(0, 0, 0.1), segs=16)
