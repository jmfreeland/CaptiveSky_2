"""Brass telescope on a wooden tripod, leather-wrapped barrel tilted toward -Y. A prop for island stargazing."""
import math

from mathutils import Vector

import lib

NAME = "Telescope"


def build(A):
    top = Vector((0, 0, 1.28))
    # tripod legs (grain along the leg), iron-shod feet
    for k in range(3):
        a = math.radians(90 + k * 120)
        foot = Vector((0.72 * math.cos(a), 0.72 * math.sin(a), 0.0))
        c = (top + foot) / 2
        d = top - foot
        A.box("wood", (0.055, 0.04, d.length), loc=tuple(c), rot=lib.align(d), bevel=0.004, name="leg")
        A.cyl("iron", 0.026, 0.07, loc=tuple(foot + Vector((0, 0, 0.03))), r2=0.01, segs=8, uv="box", name="foot")
        # leg-to-leg chain brace
        a2 = math.radians(90 + (k + 1) * 120)
        p1 = foot * 0.55 + Vector((0, 0, 0.45))
        p2 = Vector((0.72 * math.cos(a2), 0.72 * math.sin(a2), 0)) * 0.55 + Vector((0, 0, 0.45))
        A.tube("iron", [p1, (p1 + p2) / 2 + Vector((0, 0, -0.04)), p2], 0.006, segs=5, name="chain")
    # mount head
    A.cyl("brass", 0.09, 0.09, loc=(0, 0, 1.3), segs=20, name="head")
    A.cyl("brass", 0.045, 0.14, loc=(0, 0, 1.38), rot=(0, 90, 0), segs=14, name="pivot")
    # barrel: tilt 38 deg up toward -Y
    el = math.radians(38)
    d = Vector((0, -math.cos(el), math.sin(el)))
    pivot = Vector((0, 0, 1.4))
    rot = lib.align(d)
    def at(s):  # point along the barrel, s metres from the pivot (negative = towards the eyepiece)
        return tuple(pivot + d * s)
    A.cyl("brass", 0.05, 0.62, loc=at(0.0), r2=0.045, rot=rot, segs=24, name="barrelMid")
    A.cyl("brass", 0.062, 0.5, loc=at(0.52), r2=0.05, rot=rot, segs=24, name="barrelFront")   # wide objective section
    A.cyl("brass", 0.078, 0.07, loc=at(0.82), r2=0.062, rot=rot, segs=24, name="bell")
    A.cyl("leather", 0.0525, 0.3, loc=at(0.05), rot=rot, segs=24, uv="box", name="wrap")
    A.cyl("brass", 0.034, 0.34, loc=at(-0.45), r2=0.026, rot=rot, segs=20, name="eyetube")
    A.cyl("brass", 0.04, 0.05, loc=at(-0.62), rot=rot, segs=20, name="eyepiece")
    for s in (-0.28, 0.26, 0.74):
        A.torus("brass", 0.056 if s < 0.7 else 0.066, 0.008, loc=at(s), rot=rot, segs=24, rsegs=5, name="band")
    A.collide_cyl(0.75, 1.4, loc=(0, 0, 0.7), segs=10)
