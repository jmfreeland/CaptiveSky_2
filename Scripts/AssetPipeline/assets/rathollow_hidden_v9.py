"""A low fallen-log hollow with a sheltered entrance inside the trunk's front silhouette."""

import importlib.util
import os

import bpy
from mathutils import Vector


_V6_SOURCE = os.path.join(os.path.dirname(__file__), "rathollow_barkv6.py")
_V6_SPEC = importlib.util.spec_from_file_location("rathollow_hidden_v9_bark", _V6_SOURCE)
rathollow_barkv6 = importlib.util.module_from_spec(_V6_SPEC)
_V6_SPEC.loader.exec_module(rathollow_barkv6)


NAME = "RatHollow_HiddenV9"
BARK_TILE_METRES = 1.2
ASSET_ANCHORS_CM = {
    "entrance": (-132.0, 0.0, 66.0),
    "nest_bed": (-32.0, 0.0, 32.0),
}

# Keep the entrance silhouette within the hollow shell's x=-1.30 m front plane.
CANOPY_POINTS_M = (
    (-1.30, -0.31, 1.02),
    (-1.28, -0.23, 1.10),
    (-1.27, -0.04, 1.15),
    (-1.26, 0.16, 1.11),
    (-1.28, 0.30, 1.02),
)
NEAR_SHOULDER_POINTS_M = (
    (-1.24, -0.39, 0.47),
    (-1.16, -0.48, 0.35),
    (-0.98, -0.55, 0.24),
)
FAR_SHOULDER_POINTS_M = (
    (-1.22, 0.41, 0.44),
    (-1.11, 0.51, 0.32),
    (-0.94, 0.57, 0.23),
)
NEAR_GROUND_ROOT_POINTS_M = (
    (-0.92, -0.46, 0.30),
    (-0.48, -0.62, 0.24),
    (0.20, -0.63, 0.22),
    (0.82, -0.48, 0.25),
)
FAR_GROUND_ROOT_POINTS_M = (
    (-0.84, 0.42, 0.27),
    (-0.30, 0.59, 0.23),
    (0.42, 0.61, 0.23),
    (0.86, 0.43, 0.24),
)
BASE_ENTRANCE_TIPS_TO_REMOVE = ("entranceSplinter_", "splitRimEast_", "splitRimWest_")


def _remove_protruding_base_tips(A):
    kept_parts = []
    removed_names = set()
    for obj, material_key in A.parts:
        if obj.name.startswith(BASE_ENTRANCE_TIPS_TO_REMOVE):
            removed_names.add(obj.name.split("_")[0])
            bpy.data.objects.remove(obj, do_unlink=True)
        else:
            kept_parts.append((obj, material_key))
    A.parts[:] = kept_parts
    if len(removed_names) != len(BASE_ENTRANCE_TIPS_TO_REMOVE):
        raise RuntimeError("Expected to replace the three exposed base entrance tips")


def _vectors(points):
    return [Vector(point) for point in points]


def build(A):
    rathollow_barkv6._ensure_versioned_textures()
    rathollow = rathollow_barkv6.rathollow
    rathollow.lib.TILED["bark"] = ("T_Lib_RatHollow_BarkV6", BARK_TILE_METRES)
    rathollow._hollow_log(A, conform_moss=True, bark_tile=BARK_TILE_METRES)
    _remove_protruding_base_tips(A)

    # A crooked, low canopy shades the upper opening without projecting past
    # the cut end or crossing the passage.
    A.tube("bark", _vectors(CANOPY_POINTS_M), 0.036, segs=8,
           taper=lambda t: 1.0 - 0.28 * t, tile=0.9, name="shelteringRootCanopy")
    A.tube("bark", _vectors(NEAR_SHOULDER_POINTS_M), 0.056, segs=8,
           taper=lambda t: 0.92 - 0.72 * t, tile=0.9, name="entranceRootShoulderNear")
    A.tube("bark", _vectors(FAR_SHOULDER_POINTS_M), 0.052, segs=8,
           taper=lambda t: 0.9 - 0.7 * t, tile=0.9, name="entranceRootShoulderFar")

    # Retain the low oblique roots that make the trunk read as fallen wood, but
    # keep both tips tucked within its long silhouette.
    A.tube("bark", _vectors(NEAR_GROUND_ROOT_POINTS_M), 0.068, segs=8,
           taper=lambda t: 0.9 - 0.66 * t, tile=0.9, name="groundedRootNear")
    A.tube("bark", _vectors(FAR_GROUND_ROOT_POINTS_M), 0.054, segs=8,
           taper=lambda t: 0.86 - 0.64 * t, tile=0.9, name="groundedRootFar")

    rathollow._triangulate_export_meshes(A)
