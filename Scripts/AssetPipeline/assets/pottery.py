"""Terracotta pottery set: tall two-handled amphora, round pot, lidded jar."""
import math

from mathutils import Vector

NAME = "PotterySet"


def build(A):
    # amphora (outer profile up, rim, inner profile down)
    amph = [(0, 0), (0.07, 0), (0.07, 0.03), (0.13, 0.12), (0.2, 0.3), (0.22, 0.42), (0.19, 0.55), (0.11, 0.68),
            (0.075, 0.74), (0.085, 0.8), (0.115, 0.835), (0.115, 0.855), (0.085, 0.855), (0.062, 0.8), (0.058, 0.7), (0, 0.7)]
    A.lathe("clay", amph, loc=(0, 0, 0), segs=40, name="amphora", sharp=40)
    for s in (1, -1):
        path = []
        for t in range(0, 13):
            u = t / 12
            x = 0.08 + 0.15 * math.sin(u * math.pi * 0.95) + 0.015 * u
            z = 0.78 - 0.26 * u
            path.append(Vector((s * x, 0, z)))
        A.tube("clay", path, 0.022, segs=10, name="handle", taper=lambda t: 1.0 + 0.25 * math.sin(t * math.pi))
    # round pot
    pot = [(0, 0), (0.09, 0), (0.15, 0.05), (0.22, 0.17), (0.21, 0.29), (0.15, 0.36), (0.165, 0.395), (0.165, 0.42),
           (0.135, 0.42), (0.128, 0.39), (0.12, 0.3), (0, 0.28)]
    A.lathe("clay", pot, loc=(0.58, 0.18, 0), segs=40, name="pot", sharp=40)
    # lidded jar
    jar = [(0, 0), (0.11, 0), (0.135, 0.05), (0.15, 0.2), (0.125, 0.3), (0.105, 0.32), (0.105, 0.34), (0.09, 0.34), (0, 0.3)]
    A.lathe("clay", jar, loc=(-0.52, 0.12, 0), segs=36, name="jar", sharp=40)
    lid = [(0, 0.335), (0.115, 0.335), (0.112, 0.36), (0.085, 0.39), (0.04, 0.41), (0.035, 0.425), (0.045, 0.445), (0.025, 0.455), (0, 0.455)]
    A.lathe("clay", lid, loc=(-0.52, 0.12, 0), segs=36, name="lid", sharp=40)
    # shard of rope-tied neck on the jar
    A.tube("rope", [Vector((0.108 * math.cos(a), 0.108 * math.sin(a), 0.32)) for a in [i * math.pi / 12 for i in range(26)]],
           0.008, segs=6, loc=(-0.52, 0.12, 0), name="tie")
    A.collide_cyl(0.23, 0.86, loc=(0, 0, 0.43))
    A.collide_cyl(0.22, 0.42, loc=(0.58, 0.18, 0.21))
    A.collide_cyl(0.16, 0.46, loc=(-0.52, 0.12, 0.23))
