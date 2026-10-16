"""RatHollow V5 keeps the V4 shell but conforms moss to its bark surface."""
import importlib.util
import os


_SOURCE = os.path.join(os.path.dirname(__file__), "rathollow.py")
_SPEC = importlib.util.spec_from_file_location("rathollow_v4_source", _SOURCE)
rathollow = importlib.util.module_from_spec(_SPEC)
_SPEC.loader.exec_module(rathollow)


NAME = "RatHollow_BarkV5"


def build(A):
    rathollow._hollow_log(A, conform_moss=True)
    rathollow._triangulate_export_meshes(A)
