"""Blender headless: build an asset script, bake stone, export FBX + manifest.

blender -b --factory-startup -P run_asset.py -- <asset_name>      (assets/<asset_name>.py)
"""
import importlib.util
import json
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
A.description = " ".join((mod.__doc__ or "").split())
mod.build(A)
A.finalize()
provenance = getattr(mod, "PROVENANCE", None)
if provenance:
    manifest_path = os.path.join(A.dir, "manifest.json")
    with open(manifest_path, "r", encoding="utf-8") as manifest_file:
        manifest = json.load(manifest_file)
    manifest["provenance"] = provenance
    with open(manifest_path, "w", encoding="utf-8") as manifest_file:
        json.dump(manifest, manifest_file, indent=2)
