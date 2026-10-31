"""A low fallen-log hollow with a broken root canopy around its sheltered entrance."""

import importlib.util
import os

from mathutils import Vector


_V6_SOURCE = os.path.join(os.path.dirname(__file__), "rathollow_barkv6.py")
_V6_SPEC = importlib.util.spec_from_file_location("rathollow_hidden_v7_bark", _V6_SOURCE)
rathollow_barkv6 = importlib.util.module_from_spec(_V6_SPEC)
_V6_SPEC.loader.exec_module(rathollow_barkv6)


NAME = "RatHollow_HiddenV7"
ASSET_ANCHORS_CM = {
    "entrance": (-132.0, 0.0, 66.0),
    "nest_bed": (-32.0, 0.0, 32.0),
}


def build(A):
    rathollow_barkv6._ensure_versioned_textures()
    rathollow = rathollow_barkv6.rathollow
    rathollow.lib.TILED["bark"] = ("T_Lib_RatHollow_BarkV6", 1.2)
    rathollow._hollow_log(A, conform_moss=True)

    # A crooked root lip shades the top of the opening without crossing its
    # rat-sized passage. Its uneven arc breaks the near-perfect circular read.
    A.tube(
        "bark",
        [
            Vector((-1.34, -0.37, 1.05)),
            Vector((-1.42, -0.24, 1.15)),
            Vector((-1.35, -0.04, 1.20)),
            Vector((-1.40, 0.17, 1.16)),
            Vector((-1.31, 0.36, 1.04)),
        ],
        0.045,
        segs=8,
        taper=lambda t: 1.0 - 0.24 * t,
        tile=0.9,
        name="brokenRootCanopy",
    )

    # Long, low roots anchor the trunk into its surroundings and make its
    # horizontal fallen form legible from an oblique visitor view.
    A.tube(
        "bark",
        [
            Vector((-0.95, -0.48, 0.30)),
            Vector((-0.55, -0.66, 0.24)),
            Vector((0.18, -0.68, 0.25)),
            Vector((0.76, -0.52, 0.28)),
            Vector((1.10, -0.38, 0.23)),
        ],
        0.075,
        segs=8,
        taper=lambda t: 0.9 - 0.65 * t,
        tile=0.9,
        name="groundedRootNear",
    )
    A.tube(
        "bark",
        [
            Vector((-0.82, 0.43, 0.27)),
            Vector((-0.35, 0.61, 0.22)),
            Vector((0.42, 0.63, 0.24)),
            Vector((0.93, 0.46, 0.25)),
        ],
        0.060,
        segs=8,
        taper=lambda t: 0.85 - 0.62 * t,
        tile=0.9,
        name="groundedRootFar",
    )

    rathollow._triangulate_export_meshes(A)
