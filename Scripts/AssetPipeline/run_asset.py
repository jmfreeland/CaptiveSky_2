"""Blender headless: build an asset script, bake stone, export FBX + manifest.

blender -b --factory-startup -P run_asset.py -- <asset_name>      (assets/<asset_name>.py)

An asset module may define ASSET_ANCHORS_CM as named local XYZ centimetre points;
the verifier checks that they lie within the re-imported mesh bounds.
"""
import importlib.util
import json
import math
import os
import sys

PIPE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, PIPE)
name = sys.argv[sys.argv.index("--") + 1]
import lib  # noqa: E402

spec = importlib.util.spec_from_file_location(f"asset_{name}", f"{PIPE}/assets/{name}.py")
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)

provenance = getattr(mod, "PROVENANCE", None)
anchors_cm = getattr(mod, "ASSET_ANCHORS_CM", None)
metadata = {}
if provenance:
    metadata["provenance"] = provenance
if anchors_cm is not None:
    if not isinstance(anchors_cm, dict) or not anchors_cm:
        raise ValueError("ASSET_ANCHORS_CM must be a non-empty mapping of names to XYZ centimetres")
    normalized_anchors = {}
    for anchor_name, position in anchors_cm.items():
        if not isinstance(anchor_name, str) or not anchor_name.strip():
            raise ValueError("Asset anchor names must be non-empty strings")
        if not isinstance(position, (tuple, list)) or len(position) != 3:
            raise ValueError(f"Asset anchor {anchor_name!r} must contain exactly three coordinates")
        coordinates = [float(value) for value in position]
        if not all(math.isfinite(value) for value in coordinates):
            raise ValueError(f"Asset anchor {anchor_name!r} must contain only finite coordinates")
        normalized_anchors[anchor_name] = coordinates
    metadata["anchors_cm"] = normalized_anchors

A = lib.Asset(mod.NAME, getattr(mod, "BAKE_SIZE", 2048))
A.description = " ".join((mod.__doc__ or "").split())
A.fbx_mesh_smooth_type = getattr(mod, "FBX_MESH_SMOOTH_TYPE", "OFF")
A.fbx_use_tspace = getattr(mod, "FBX_USE_TSPACE", True)
mod.build(A)
A.finalize()
if metadata:
    manifest_path = os.path.join(A.dir, "manifest.json")
    with open(manifest_path, "r", encoding="utf-8") as manifest_file:
        manifest = json.load(manifest_file)
    manifest.update(metadata)
    with open(manifest_path, "w", encoding="utf-8") as manifest_file:
        json.dump(manifest, manifest_file, indent=2)
