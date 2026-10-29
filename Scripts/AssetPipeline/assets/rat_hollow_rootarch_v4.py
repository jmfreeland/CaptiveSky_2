"""A softer, more weathered RatHollow candidate for woodland review."""

import importlib.util
import math
from pathlib import Path

import lib


_BASE_PATH = Path(__file__).with_name("rat_hollow_rootarch_v3.py")
_BASE_SPEC = importlib.util.spec_from_file_location("rat_hollow_rootarch_v3_base", _BASE_PATH)
_BASE = importlib.util.module_from_spec(_BASE_SPEC)
_BASE_SPEC.loader.exec_module(_BASE)

NAME = "RatHollowRootArchBurrowWeatheredV4"
ASSET_ANCHORS_CM = {
    "entrance_center": (-78.0, -1.0, 26.0),
    "entrance_threshold": (-78.0, -1.0, 3.0),
}

# Keep V3's bark tile scale; the existing bark source becomes oversized when
# stretched further across the log.
BARK_TEXTURE_TILE_METRES = 1.2
HOLLOWWOOD_BASE_COLOR = (0.47, 0.31, 0.17)

# Moss follows the crown of the log in broken, overlapping patches rather than
# forming a blanket. Each tuple is (x, normalized lateral offset, radius).
MOSS_CROWN_PATCHES = (
    (-0.50, -0.20, 0.18),
    (-0.25, 0.12, 0.20),
    (0.00, -0.12, 0.21),
    (0.27, 0.16, 0.19),
    (0.52, -0.05, 0.16),
)


def _add_moss_crown(A):
    for index, (x, normalized_y, radius) in enumerate(MOSS_CROWN_PATCHES):
        alpha = (x + 0.78) / 1.56
        center_y = -0.01 + 0.06 * alpha
        radius_y = 0.45 - 0.12 * alpha
        center_z = 0.47 - 0.08 * alpha
        radius_z = 0.43 - 0.08 * alpha
        y = center_y + normalized_y * radius_y
        crown_z = center_z + radius_z * math.sqrt(max(0.0, 1.0 - normalized_y * normalized_y))
        A.rock(
            "moss",
            (radius * 1.28, radius * 0.70, 0.014),
            loc=(x, y, crown_z + 0.004),
            seed=101 + index * 17,
            sub=2,
            rough=0.08,
            flat_bottom=0.90,
            tile=0.38,
            name="mossCrown{:02d}".format(index),
        )


def build(A):
    lib.TILED["bark"] = ("T_Lib_RatHollow_Bark", BARK_TEXTURE_TILE_METRES)
    _BASE.HOLLOWWOOD_BASE_COLOR = HOLLOWWOOD_BASE_COLOR
    _BASE.build(A)
    _add_moss_crown(A)
