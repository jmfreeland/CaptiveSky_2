"""A low fallen-log hollow with uneven root shoulders framing its entrance."""

import importlib.util
import os

from mathutils import Vector


_V7_SOURCE = os.path.join(os.path.dirname(__file__), "rathollow_hidden_v7.py")
_V7_SPEC = importlib.util.spec_from_file_location("rathollow_hidden_v8_base", _V7_SOURCE)
rathollow_hidden_v7 = importlib.util.module_from_spec(_V7_SPEC)
_V7_SPEC.loader.exec_module(rathollow_hidden_v7)


NAME = "RatHollow_HiddenV8"
ASSET_ANCHORS_CM = dict(rathollow_hidden_v7.ASSET_ANCHORS_CM)


def build(A):
    # Preserve V7's horizontal trunk, protected entrance, texture set, named
    # anchors, and five simple colliders. V8 changes only the entrance silhouette.
    rathollow_hidden_v7.build(A)

    # Two asymmetrical, ground-hugging root shoulders break up the circular
    # entrance rim. They stay to either side of the rat-sized passage, then
    # flow back along the trunk instead of forming a blocking sill.
    A.tube(
        "bark",
        [
            Vector((-1.27, -0.40, 0.48)),
            Vector((-1.34, -0.51, 0.39)),
            Vector((-1.16, -0.60, 0.28)),
            Vector((-0.86, -0.65, 0.22)),
        ],
        0.068,
        segs=8,
        taper=lambda t: 0.92 - 0.76 * t,
        tile=0.9,
        name="entranceRootShoulderNear",
    )
    A.tube(
        "bark",
        [
            Vector((-1.21, 0.40, 0.44)),
            Vector((-1.39, 0.50, 0.35)),
            Vector((-1.20, 0.60, 0.25)),
            Vector((-0.91, 0.63, 0.21)),
        ],
        0.061,
        segs=8,
        taper=lambda t: 0.88 - 0.72 * t,
        tile=0.9,
        name="entranceRootShoulderFar",
    )

    rathollow_hidden_v7.rathollow_barkv6.rathollow._triangulate_export_meshes(A)
