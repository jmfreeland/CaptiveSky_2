"""Standing stone with glowing runes carved into its (-Y) face. Runes are real geometry, emissive."""
import math

import bpy
from mathutils import Matrix, Vector

NAME = "RuneStone"
BAKE_SIZE = 2048

# glyphs as segments in a unit box: x in [-0.5, 0.5], y in [-1, 1]
GLYPHS = [
    [((0, -1), (0, 1)), ((0, 1), (0.45, 0.45)), ((0, 0.4), (0.45, -0.15))],
    [((-0.4, -1), (-0.4, 1)), ((-0.4, 1), (0.4, 0.2)), ((0.4, 0.2), (0.4, -1))],
    [((0, -1), (0, 1)), ((0, 0.55), (0.45, 0.15)), ((0.45, 0.15), (0, -0.25))],
    [((0, -1), (0, 1)), ((0, 1), (0.45, 0.55)), ((0, 0.4), (0.45, -0.05))],
    [((-0.4, -1), (-0.4, 1)), ((-0.4, 1), (0.4, 0.55)), ((0.4, 0.55), (-0.4, 0.05)), ((-0.4, 0.05), (0.4, -1))],
    [((0.4, 1), (-0.4, 0)), ((-0.4, 0), (0.4, -1))],
    [((-0.4, 1), (0.4, -1)), ((0.4, 1), (-0.4, -1))],
    [((0, -1), (0, 1)), ((-0.4, 0.6), (0.4, 0.6)), ((-0.4, -0.2), (0.4, -0.2))],
]


def build(A):
    st = A.rock("stone", (0.46, 0.28, 1.15), loc=(0, 0, 1.0), rot=(2, 0, 8), seed=4, sub=4, rough=0.2, flat_bottom=0.82, tile=1.2, name="menhir")
    A.rock("stone", (0.22, 0.18, 0.12), loc=(0.45, -0.25, 0.09), rot=(0, 0, 40), seed=11, tile=1.2, name="foot")
    A.rock("stone", (0.18, 0.16, 0.1), loc=(-0.4, -0.3, 0.07), rot=(0, 0, -30), seed=13, tile=1.2, name="foot")
    A.rock("stone", (0.15, 0.13, 0.09), loc=(-0.2, 0.35, 0.06), seed=15, tile=1.2, name="foot")
    bpy.context.view_layer.update()
    ev = st.evaluated_get(bpy.context.evaluated_depsgraph_get())
    zs = [0.62, 0.92, 1.22, 1.5, 1.78, 2.05]
    xs = [0.02, -0.04, 0.03, -0.02, 0.02, -0.01]
    for i, (z, x) in enumerate(zip(zs, xs)):
        ok, loc, nrm, _ = ev.ray_cast(Vector((x, -2.0, z)), Vector((0, 1, 0)), distance=4.0)
        if not ok:
            print("rune", i, "missed")
            continue
        nrm = nrm.normalized()
        right = Vector((1, 0, 0)) - nrm * nrm.x
        right.normalize()
        up = nrm.cross(right).normalized()
        if up.z < 0:
            up = -up
        right = up.cross(nrm).normalized()
        frame = Matrix((right, up, nrm)).transposed()
        sx = sz = 0.085
        for (a, b) in GLYPHS[i % len(GLYPHS)]:
            p0 = Vector((a[0] * sx, a[1] * sz, 0))
            p1 = Vector((b[0] * sx, b[1] * sz, 0))
            mid = (p0 + p1) / 2
            seg = p1 - p0
            wp = loc + right * mid.x + up * mid.y - nrm * 0.004
            e = (frame @ Matrix.Rotation(math.atan2(seg.y, seg.x), 3, "Z")).to_euler("XYZ")
            A.box("rune", (seg.length + 0.014, 0.017, 0.016), loc=tuple(wp), rot=tuple(math.degrees(v) for v in e), name="rune")
    A.collide_cyl(0.42, 2.1, loc=(0, 0, 1.05), segs=12)
