"""Convert baked/derived maps into Unreal-ready textures and write the import manifest.

Run: python -I pack_textures.py
  Stone (baked atlas):  T_Well_Stone_BC / _N (DirectX, green flipped) / _ORM (R=AO G=Rough B=Metal)
  Wood (tileable):      T_Well_Wood_BC (warmed to match the shader) / _N / _ORM
"""
import json
import os

import numpy as np
from PIL import Image

WORK = "D:/comfy_projects/textures/bake"
SRC = "D:/comfy_projects/textures/outputs"
OUT = "D:/Projects - Athena/Unreal/CaptiveSky_2/Saved/CaptiveSky/ComfyBlender/Well"
TEX = f"{OUT}/Textures"
os.makedirs(TEX, exist_ok=True)


def load(p, mode="RGB"):
    return Image.open(p).convert(mode)


def gray(p):
    return np.asarray(load(p, "L"), dtype=np.float32) / 255.0


def save_arr(a, path):
    Image.fromarray((np.clip(a, 0, 1) * 255 + 0.5).astype(np.uint8)).save(path)


# ---- stone (baked)
bc = load(f"{WORK}/stone_bc.png")
bc.save(f"{TEX}/T_Well_Stone_BC.png")
n = np.asarray(load(f"{WORK}/stone_normal_gl.png"), dtype=np.float32) / 255.0
n[..., 1] = 1.0 - n[..., 1]  # OpenGL (+Y) -> DirectX (-Y) for Unreal
save_arr(n, f"{TEX}/T_Well_Stone_N.png")
ao, rough = gray(f"{WORK}/stone_ao.png"), gray(f"{WORK}/stone_rough.png")
save_arr(np.stack([ao, rough, np.zeros_like(ao)], axis=-1), f"{TEX}/T_Well_Stone_ORM.png")

# ---- wood (tileable). Reproduce the shader's warm-up: sat*1.25, value*0.88, multiply (1,.90,.76)
w = np.asarray(load(f"{SRC}/wood/albedo.png"), dtype=np.float32) / 255.0
hsv = np.asarray(load(f"{SRC}/wood/albedo.png").convert("HSV"), dtype=np.float32) / 255.0
hsv[..., 1] = np.clip(hsv[..., 1] * 1.25, 0, 1)
hsv[..., 2] = np.clip(hsv[..., 2] * 0.88, 0, 1)
w = np.asarray(Image.fromarray((hsv * 255 + 0.5).astype(np.uint8), "HSV").convert("RGB"), dtype=np.float32) / 255.0
w = w * np.array([1.0, 0.90, 0.76], dtype=np.float32)
save_arr(w, f"{TEX}/T_Well_Wood_BC.png")
Image.open(f"{SRC}/wood/normal_dx.png").convert("RGB").save(f"{TEX}/T_Well_Wood_N.png")
Image.open(f"{SRC}/wood/orm.png").convert("RGB").save(f"{TEX}/T_Well_Wood_ORM.png")

manifest = {
    "asset": "SM_Well_01",
    "fbx": "SM_Well_01.fbx",
    "units": "centimetres (FBX_SCALE_ALL); origin = ground-centre of the well; Z up",
    "size_cm_xyz": [245, 204, 261],
    "collision": "UCX_SM_Well_01_01 convex cylinder embedded in the FBX (wall only, r=86 cm, h=90 cm)",
    "uv": "single UV channel 'UVMap'. Stone slot: unique 0-1 atlas. Wood slot: tiled (>1) UVs, texture must wrap.",
    "materials": {
        "M_Well_Stone": {
            "BaseColor": "Textures/T_Well_Stone_BC.png (sRGB)",
            "Normal": "Textures/T_Well_Stone_N.png (DirectX, compression: Normalmap)",
            "ORM": "Textures/T_Well_Stone_ORM.png (R=AO, G=Roughness, B=Metallic; sRGB off, compression: Masks)",
            "notes": "moss, stone variation and crevice AO are baked in; no tiling.",
        },
        "M_Well_Wood": {
            "BaseColor": "Textures/T_Well_Wood_BC.png (sRGB, tiles)",
            "Normal": "Textures/T_Well_Wood_N.png (DirectX, tiles)",
            "ORM": "Textures/T_Well_Wood_ORM.png (R=AO, G=Roughness, B=Metallic; sRGB off)",
        },
        "M_Well_Mortar": {"BaseColor": [0.07, 0.065, 0.06], "Roughness": 0.95},
        "M_Well_Iron": {"BaseColor": [0.16, 0.15, 0.15], "Roughness": 0.4, "Metallic": 0.9},
        "M_Well_Rope": {"BaseColor": [0.32, 0.22, 0.12], "Roughness": 0.92},
        "M_Well_Water": {"BaseColor": [0.02, 0.06, 0.08], "Roughness": 0.05,
                         "notes": "static disc at z=45 cm; swap for the project's water/translucent material."},
    },
    "provenance": "Textures generated locally with ComfyUI (SDXL base 1.0) + derived PBR maps (pbr.py); modelled/baked in Blender 5.2. Sources and scripts: D:/comfy_projects/textures",
}
json.dump(manifest, open(f"{OUT}/manifest.json", "w"), indent=2)
print("packed", sorted(os.listdir(TEX)))
