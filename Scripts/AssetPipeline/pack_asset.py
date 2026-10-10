"""Venv python: turn raw stone bakes into Unreal textures. python -I pack_asset.py <Folder>"""
import os
import sys

import numpy as np
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from config import ROOT, WORK  # noqa: E402

folder = sys.argv[1]
OUT = f"{ROOT}/{folder}/Textures"
os.makedirs(OUT, exist_ok=True)
p = lambda k: f"{WORK}/{folder}_stone_{k}.png"
if not os.path.exists(p("bc")):
    print("no stone bake for", folder)
    sys.exit(0)


def gray(path):
    return np.asarray(Image.open(path).convert("L"), dtype=np.float32) / 255.0


def save(a, path):
    Image.fromarray((np.clip(a, 0, 1) * 255 + 0.5).astype(np.uint8)).save(path)


Image.open(p("bc")).convert("RGB").save(f"{OUT}/T_{folder}_Stone_BC.png")
n = np.asarray(Image.open(p("normal_gl")).convert("RGB"), dtype=np.float32) / 255.0
n[..., 1] = 1.0 - n[..., 1]  # OpenGL -> DirectX
save(n, f"{OUT}/T_{folder}_Stone_N.png")
ao, rough = gray(p("ao")), gray(p("rough"))
save(np.stack([ao, rough, np.zeros_like(ao)], axis=-1), f"{OUT}/T_{folder}_Stone_ORM.png")
print("packed stone textures for", folder, "ao mean", round(float(ao.mean()), 3), "rough mean", round(float(rough.mean()), 3))
