"""Add a tileable texture to the shared library from a pbr.py output folder.

python -I pack_library.py <Key> <pbr_dir>
  e.g. python -I pbr.py clay_src.png out/clay --normal-strength 3.0
       python -I pack_library.py Clay out/clay
Writes <export root>/Library/Textures/T_Lib_<Key>_{BC,N,ORM}.png (DirectX normal, ORM = AO/Rough/Metal).
Then add the key to TILED in lib.py (texture base name + tile size in metres) so asset scripts can use it.
"""
import os
import sys

from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from config import ROOT  # noqa: E402

key, src = sys.argv[1], sys.argv[2]
out = f"{ROOT}/Library/Textures"
os.makedirs(out, exist_ok=True)
for suffix, name in (("BC", "albedo"), ("N", "normal_dx"), ("ORM", "orm")):
    Image.open(f"{src}/{name}.png").convert("RGB").save(f"{out}/T_Lib_{key}_{suffix}.png")
print("library texture written:", key)
