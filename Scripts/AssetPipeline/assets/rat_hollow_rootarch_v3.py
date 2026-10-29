"""Lighter, more legible fallen-root shelter for Rat's hidden home."""

import importlib.util
from pathlib import Path

import lib


_BASE_PATH = Path(__file__).with_name("rat_hollow_rootarch_v2.py")
_BASE_SPEC = importlib.util.spec_from_file_location("rat_hollow_rootarch_v2_base", _BASE_PATH)
_BASE = importlib.util.module_from_spec(_BASE_SPEC)
_BASE_SPEC.loader.exec_module(_BASE)

NAME = "RatHollowRootArchBurrowLightwellV3"
ASSET_ANCHORS_CM = {
    "entrance_center": (-78.0, -1.0, 26.0),
    "entrance_threshold": (-78.0, -1.0, 3.0),
}

# The older preview read as a black circular hole. Open more of the lower rim
# and enlarge the arch modestly while preserving the rat-scale passage.
RIM_SEGMENT_COUNT = 20
ENTRANCE_RIM_GAP_SEGMENTS = (11, 12, 13, 14, 15, 16, 17, 18)
ENTRANCE_FRONT_X_M = -0.78
ENTRANCE_CENTER_Y_M = -0.01
ENTRANCE_CENTER_Z_M = 0.26
ENTRANCE_MOUTH_RINGS = (
    (-0.78, -0.01, 0.47, 0.275, 0.275, -0.01, 0.26, 0.20, 0.23),
    (-0.60, -0.01, 0.47, 0.265, 0.260, -0.01, 0.32, 0.225, 0.245),
    (-0.34,  0.00, 0.46, 0.245, 0.240,  0.00, 0.38, 0.235, 0.230),
)

# Warmer mid-value inner wood keeps the sheltered recess readable in daylight
# without adding artificial emissive light to a natural wildlife home.
HOLLOWWOOD_BASE_COLOR = (0.34, 0.21, 0.11)


def build(A):
    lib.PLAIN["hollowwood"] = {
        "BaseColor": list(HOLLOWWOOD_BASE_COLOR),
        "Roughness": 0.95,
    }
    _BASE.RIM_SEGMENT_COUNT = RIM_SEGMENT_COUNT
    _BASE.ENTRANCE_RIM_GAP_SEGMENTS = ENTRANCE_RIM_GAP_SEGMENTS
    _BASE.ENTRANCE_FRONT_X_M = ENTRANCE_FRONT_X_M
    _BASE.ENTRANCE_CENTER_Y_M = ENTRANCE_CENTER_Y_M
    _BASE.ENTRANCE_CENTER_Z_M = ENTRANCE_CENTER_Z_M
    _BASE.ENTRANCE_MOUTH_RINGS = ENTRANCE_MOUTH_RINGS
    _BASE.build(A)
