"""Same RatHollow silhouette as V9, testing finer bark texture scale."""

import importlib.util
import os


_V9_SOURCE = os.path.join(os.path.dirname(__file__), "rathollow_hidden_v9.py")
_V9_SPEC = importlib.util.spec_from_file_location("rathollow_hidden_v11_base", _V9_SOURCE)
rathollow_hidden_v9 = importlib.util.module_from_spec(_V9_SPEC)
_V9_SPEC.loader.exec_module(rathollow_hidden_v9)


NAME = "RatHollow_HiddenV11"
BARK_TILE_METRES = 0.8
ASSET_ANCHORS_CM = dict(rathollow_hidden_v9.ASSET_ANCHORS_CM)


def build(A):
    # Isolate a single material-scale change against V9: texture frequency on
    # the trunk bark, with geometry, colliders, and anchor positions preserved.
    rathollow_hidden_v9.BARK_TILE_METRES = BARK_TILE_METRES
    rathollow_hidden_v9.build(A)
