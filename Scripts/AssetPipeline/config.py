"""Paths shared by every pipeline script (importable from Blender's Python and from the venv).

Everything is derived from the repo location, so a checkout works anywhere. Override with env vars:
  ASSET_EXPORT_ROOT   where assets are exported       (default <repo>/Saved/CaptiveSky/ComfyBlender)
  ASSET_WORK          raw bake intermediates          (default <export root>/_work)
  ASSET_SOURCES       stone-face + moss source maps   (default <export root>/Library/Sources)
  BLENDER_EXE         Blender 5.2 executable
`Saved/` is gitignored, so exports, bakes and source maps are local; only the scripts live in git.
"""
import os

PIPE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(PIPE, "..", ".."))
ROOT = os.environ.get("ASSET_EXPORT_ROOT", f"{REPO}/Saved/CaptiveSky/ComfyBlender").replace("\\", "/")
WORK = os.environ.get("ASSET_WORK", f"{ROOT}/_work").replace("\\", "/")
SRC_TEX = os.environ.get("ASSET_SOURCES", f"{ROOT}/Library/Sources").replace("\\", "/")
BLENDER = os.environ.get("BLENDER_EXE", "C:/Program Files/Blender Foundation/Blender 5.2/blender.exe")
