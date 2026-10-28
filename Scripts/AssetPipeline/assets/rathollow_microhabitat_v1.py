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
_STRAWBERRY = _load("woodland_wildstrawberry_microhabitat", "woodland_wildstrawberry.py")

# The enclosed surface should read as sheltered timber instead of an unlit void.
# Keep the colour earthy; the straw bed remains the brightest interior detail.
lib.PLAIN["hollowwood"] = {"BaseColor": [0.38, 0.24, 0.13], "Roughness": 0.96}

NAME = "RatHollowMicrohabitatV1"
ASSET_ANCHORS_CM = {
    "entrance_center": (-78.0, -1.0, 24.0),
    "entrance_threshold": (-78.0, -1.0, 3.0),
}

# Species, position, ground height, scale, yaw, and conservative plant reach.
# Low strawberries break up the log-to-ground edge; one fern adds a taller,
# asymmetric screen. The nearest rendered leaves remain outside the doorway.
VEGETATION_PLACEMENTS = (
    ("strawberry", -0.25, -0.52, 0.0, 0.80, -0.30, 0.42),
    ("fern", -0.50, 0.64, 0.0, 0.68, 1.30, 0.64),
)
MIN_ENTRANCE_CLEARANCE_M = 0.25


def _add_vegetation(A, module, x, y, z, scale, yaw):
    first_part = len(A.parts)
    module.build(A)
    rotation = math.radians(yaw)
    for obj, _material in A.parts[first_part:]:
        obj.scale *= scale
        obj.rotation_euler.z += rotation
        obj.location = Vector((x, y, z))


def build(A):
    _HOLLOW.build(A)

    # Keep the opening anchor and all eight shelter collision boxes unchanged.
    # Understory meshes are visual-only and never enter A.coll.
    A.rock("straw", (0.18, 0.125, 0.018), loc=(-0.60, -0.01, 0.05),
           seed=89, sub=1, rough=0.05, flat_bottom=0.92, tile=0.4,
           name="entryBedding")
    for species, x, y, z, scale, yaw, _reach in VEGETATION_PLACEMENTS:
        module = _STRAWBERRY if species == "strawberry" else _FERN
        _add_vegetation(A, module, x, y, z, scale, yaw)
