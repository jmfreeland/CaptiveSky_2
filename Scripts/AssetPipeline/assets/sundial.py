"""Garden sundial: mossy stone pedestal with a brass dial plate, hour ticks and a gnomon."""
import math

from mathutils import Vector

NAME = "Sundial"
BAKE_SIZE = 2048


def build(A):
    A.moss = (0.55, 0.5)
    A.cyl("stone", 0.34, 0.1, loc=(0, 0, 0.05), segs=8, tile=1.2, name="base")
    A.cyl("stone", 0.25, 0.06, loc=(0, 0, 0.13), segs=8, tile=1.2, name="plinth")
    A.lathe("stone", [(0, 0.16), (0.14, 0.16), (0.115, 0.3), (0.105, 0.5), (0.12, 0.62), (0.17, 0.7), (0.22, 0.73), (0, 0.73)],
            segs=8, tile=1.2, smooth=False, name="shaft")
    A.cyl("stone", 0.32, 0.07, loc=(0, 0, 0.765), segs=8, tile=1.2, name="top")
    # brass dial plate, rim, ticks, gnomon
    A.cyl("brass", 0.285, 0.016, loc=(0, 0, 0.8), segs=40, uv="box", name="plate")
    A.torus("brass", 0.285, 0.012, loc=(0, 0, 0.808), segs=40, rsegs=6, name="rim")
    for k in range(12):
        a = math.radians(k * 30)
        big = k % 3 == 0
        r = 0.235
        A.box("brass", (0.012 if big else 0.008, 0.07 if big else 0.045, 0.012), loc=(r * math.sin(a), r * math.cos(a), 0.812),
              rot=(0, 0, -math.degrees(a)), name="tick")
    # triangular gnomon: a quad whose top edge is almost a point
    A.cloth("brass", [[Vector((0, -0.14, 0.808)), Vector((0, 0.12, 0.808))], [Vector((0, 0.108, 0.97)), Vector((0, 0.12, 0.97))]], thick=0.012, name="gnomon")
    A.box("brass", (0.03, 0.03, 0.02), loc=(0, 0.12, 0.82), name="gnomonFoot")
    A.collide_cyl(0.34, 0.84, loc=(0, 0, 0.42), segs=8)
