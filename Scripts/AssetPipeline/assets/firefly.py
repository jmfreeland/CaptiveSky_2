"""A small, stylized firefly body with a readable carapace, legs and antennae."""
import bmesh
from mathutils import Vector


NAME = "Firefly"


def build(A):
    # Keep the authored parts around the actor origin: the runtime light is a
    # separate, animated abdomen at local X=-1.8 cm.
    A.rock("dirt", (0.036, 0.019, 0.017), loc=(-0.030, 0.0, 0.022),
           seed=17, sub=2, rough=0.035, flat_bottom=0.0, name="abdomenShell")
    A.rock("leather", (0.019, 0.014, 0.013), loc=(0.010, 0.0, 0.024),
           seed=23, sub=2, rough=0.025, flat_bottom=0.0, name="thorax")
    A.rock("char", (0.012, 0.010, 0.009), loc=(0.041, 0.0, 0.027),
           seed=29, sub=2, rough=0.02, flat_bottom=0.0, name="head")

    # Six fine, jointed legs: three pairs taper toward their small feet.
    for pair, x in enumerate((-0.012, 0.002, 0.015)):
        sweep = (-0.006, 0.001, 0.008)[pair]
        for side in (-1, 1):
            A.tube("char", [
                Vector((x, side * 0.010, 0.019)),
                Vector((x + sweep, side * 0.020, 0.012)),
                Vector((x + sweep * 1.6, side * 0.030, 0.005)),
            ], 0.00115, segs=6, taper=lambda t: 1.0 - 0.48 * t,
               name="leg")

    # Short, gently raised antennae and dark compound eyes help the head read
    # even in the night-time close view.
    for side in (-1, 1):
        A.tube("char", [
            Vector((0.047, side * 0.004, 0.031)),
            Vector((0.054, side * 0.008, 0.039)),
            Vector((0.064, side * 0.014, 0.041)),
        ], 0.0009, segs=5, taper=lambda t: 1.0 - 0.62 * t,
           name="antenna")
        A.rock("char", (0.0040, 0.0024, 0.0032),
               loc=(0.045, side * 0.0082, 0.029), seed=41 + side,
               sub=1, rough=0.0, flat_bottom=0.0, name="eye")

    # The procedural primitives intentionally keep some quads/ngons for easy
    # authoring. Triangulate this small final mesh before FBX export so tangent
    # generation is deterministic in Unreal and Blender emits no n-gon warning.
    for obj, _material in A.parts:
        bm = bmesh.new()
        bm.from_mesh(obj.data)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(obj.data)
        bm.free()
