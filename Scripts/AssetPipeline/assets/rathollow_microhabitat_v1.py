"""A Rat-scale fallen-log shelter with sparse, collisionless woodland understory."""

import importlib.util
import math
from pathlib import Path

from mathutils import Vector

import lib


_ASSET_DIR = Path(__file__).resolve().parent


def _load(name, filename):
    spec = importlib.util.spec_from_file_location(name, _ASSET_DIR / filename)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


_HOLLOW = _load("rathollow_rootarch_v2_microhabitat", "rat_hollow_rootarch_v2.py")
_FERN = _load("woodland_fern_microhabitat", "woodland_fern.py")

NAME = "RatHollowMicrohabitatV1"
ASSET_ANCHORS_CM = {
    "entrance_center": (-78.0, -1.0, 24.0),
    "entrance_threshold": (-78.0, -1.0, 3.0),
}

# Each fern is a modest, collisionless patch. The closest frond tip remains
# clear of the rat-sized opening, while the asymmetric placements break the
# bare log silhouette without building an opaque ring around the entrance.
FERN_PLACEMENTS = (
    (-0.55, -0.74, 0.0, 0.70, -0.35),
    (-0.38, 0.76, 0.0, 0.58, 0.90),
    (0.92, -0.60, 0.0, 0.62, 2.15),
)
MAX_FERN_REACH_M = 0.64
MIN_ENTRANCE_CLEARANCE_M = 0.25


def _add_fern(A, x, y, z, scale, yaw):
    first_part = len(A.parts)
    _FERN.build(A)
    rotation = math.radians(yaw)
    for obj, _material in A.parts[first_part:]:
        obj.scale *= scale
        obj.rotation_euler.z += rotation
        obj.location = Vector((x, y, z))


def build(A):
    _HOLLOW.build(A)

    # Keep the opening anchor and all eight shelter collision boxes unchanged.
    # Understory meshes are visual-only and never enter A.coll.
    for x, y, z, scale, yaw in FERN_PLACEMENTS:
        _add_fern(A, x, y, z, scale, yaw)
