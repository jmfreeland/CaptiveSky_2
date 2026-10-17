"""A fallen, hollow woodland trunk with a ground-entry Rat burrow."""

import importlib.util
import math
from pathlib import Path


_BASE_PATH = Path(__file__).with_name("rat_hollow_rootarch.py")
_BASE_SPEC = importlib.util.spec_from_file_location("rat_hollow_rootarch_base", _BASE_PATH)
_BASE = importlib.util.module_from_spec(_BASE_SPEC)
_BASE_SPEC.loader.exec_module(_BASE)
_BASE_TRUNK_SHELL = _BASE._trunk_shell
_BASE_INTERIOR_SHELL = _BASE._interior_shell

NAME = "RatHollowRootArchBurrow"
ASSET_ANCHORS_CM = {
    "entrance_center": (-78.0, -1.0, 38.0),
    "entrance_threshold": (-78.0, -1.0, 3.0),
}

RIM_SEGMENT_COUNT = 20
ENTRANCE_RIM_GAP_SEGMENTS = (12, 13, 14, 15, 16, 17)
ENTRANCE_FRONT_X_M = -0.78
ENTRANCE_CENTER_Y_M = -0.01
ENTRANCE_CENTER_Z_M = 0.47

# Match the original tunnel stations and lower only their underside into a
# shallow, continuous floor ramp; the upper arch, outer bark, and overall size
# remain unchanged.
INTERIOR_FLOOR_STATIONS = (
    (-0.78, 0.47, 0.275, 0.03),
    (-0.60, 0.47, 0.260, 0.04),
    (-0.34, 0.46, 0.240, 0.06),
    (-0.04, 0.44, 0.220, 0.09),
    (0.25, 0.42, 0.200, 0.12),
    (0.51, 0.40, 0.170, 0.16),
    (0.67, 0.395, 0.085, 0.30),
)

# UBX colliders leave a 56 cm central corridor for the 16 x 24 cm Rat capsule.
# Four small floor sections follow the interior ramp instead of creating one
# high threshold; the final box closes the tunnel before its tapered end.
COLLISION_BOXES = (
    ((1.24, 0.06, 0.69), (-0.04, -0.31, 0.385)),  # left wall
    ((1.24, 0.06, 0.69), (-0.04, 0.31, 0.385)),   # right wall
    ((1.24, 0.58, 0.06), (-0.04, 0.00, 0.76)),   # roof
    ((0.42, 0.56, 0.04), (-0.47, 0.00, 0.02)),   # floor: 4 cm
    ((0.40, 0.56, 0.04), (-0.10, 0.00, 0.06)),   # floor: 8 cm
    ((0.34, 0.56, 0.04), (0.25, 0.00, 0.10)),    # floor: 12 cm
    ((0.30, 0.56, 0.04), (0.52, 0.00, 0.14)),    # floor: 16 cm
    ((0.06, 0.58, 0.69), (0.65, 0.00, 0.385)),   # closed back
)

NEST_DETAIL_Z_OFFSET_M = -0.17
NEST_BED_SCALE = (1.5, 1.4, 1.25)
REAR_SOIL_Z_OFFSET_M = 0.09


def _trunk_shell_with_broken_ground_entry():
    bm = _BASE_TRUNK_SHELL()
    gap_segments = set(ENTRANCE_RIM_GAP_SEGMENTS)
    for face in list(bm.faces):
        # The source rim is deliberately irregular in X by up to 1.2 cm.
        # A tight planar test silently misses every annular face.
        if not all(abs(vertex.co.x - ENTRANCE_FRONT_X_M) < 0.03 for vertex in face.verts):
            continue
        center = face.calc_center_median()
        angle = math.atan2(
            center.z - ENTRANCE_CENTER_Z_M,
            center.y - ENTRANCE_CENTER_Y_M,
        ) % math.tau
        segment = int(angle / math.tau * RIM_SEGMENT_COUNT) % RIM_SEGMENT_COUNT
        if segment in gap_segments:
            bm.faces.remove(face)
    return bm


def _interior_shell_with_ground_ramp():
    bm = _BASE_INTERIOR_SHELL()
    for vertex in bm.verts:
        station = min(INTERIOR_FLOOR_STATIONS, key=lambda item: abs(item[0] - vertex.co.x))
        x, center_z, radius_z, floor_z = station
        if abs(vertex.co.x - x) > 1e-3 or vertex.co.z >= center_z:
            continue
        lower_arc_alpha = min(1.0, (center_z - vertex.co.z) / radius_z)
        vertex.co.z = center_z - lower_arc_alpha * (center_z - floor_z)
    return bm


def build(A):
    _BASE._trunk_shell = _trunk_shell_with_broken_ground_entry
    _BASE._interior_shell = _interior_shell_with_ground_ramp
    _BASE.build(A)

    # Seat the straw and soil on the lowered interior floor rather than leaving
    # them suspended at the old tunnel height.
    for obj, _material in A.parts:
        if obj.name.startswith("frontNestBed_"):
            obj.location.z += NEST_DETAIL_Z_OFFSET_M
            obj.scale.x *= NEST_BED_SCALE[0]
            obj.scale.y *= NEST_BED_SCALE[1]
            obj.scale.z *= NEST_BED_SCALE[2]
        elif obj.name.startswith("nestTwig"):
            obj.location.z += NEST_DETAIL_Z_OFFSET_M
        elif obj.name.startswith("rearSoil_"):
            obj.location.z += REAR_SOIL_Z_OFFSET_M

    for size, location in COLLISION_BOXES:
        A.collide_box(size, loc=location)
