"""Blender headless: build an asset script, bake stone, export FBX + manifest.

blender -b --factory-startup -P run_asset.py -- <asset_name>      (assets/<asset_name>.py)
"""
import importlib.util
import os
import sys

PIPE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, PIPE)
name = sys.argv[sys.argv.index("--") + 1]
import lib  # noqa: E402

spec = importlib.util.spec_from_file_location(f"asset_{name}", f"{PIPE}/assets/{name}.py")
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)
A = lib.Asset(mod.NAME, getattr(mod, "BAKE_SIZE", 2048))
mod.build(A)
A.finalize()
