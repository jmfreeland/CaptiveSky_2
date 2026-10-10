"""Derive tileable PBR maps from a generated albedo image.

Usage: python -I pbr.py <albedo.png> <out_dir> [--normal-strength 4.0] [--delight 0.7]

Writes into <out_dir>:
  albedo.png         de-lit colour (low-frequency lighting flattened)
  height.png         greyscale height estimate
  normal_gl.png      normal map, +Y up (Blender / OpenGL)
  normal_dx.png      normal map, -Y up (Unreal / DirectX)
  roughness.png      greyscale roughness
  ao.png             cavity-based ambient occlusion
  orm.png            Unreal packed: R=AO, G=Roughness, B=Metallic (0)

Everything uses wrap-around filtering, so tileable inputs stay tileable.
The maps are heuristics derived from one colour image, not measured data.
"""
import argparse
import os

import numpy as np
from PIL import Image
from scipy.ndimage import gaussian_filter


def blur(a, sigma):
    return gaussian_filter(a, sigma=sigma, mode="wrap")


def norm01(a, lo=1, hi=99):
    l, h = np.percentile(a, [lo, hi])
    return np.clip((a - l) / max(h - l, 1e-6), 0, 1)


def save(a, path):
    Image.fromarray((np.clip(a, 0, 1) * 255 + 0.5).astype(np.uint8)).save(path)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("albedo")
    ap.add_argument("out_dir")
    ap.add_argument("--normal-strength", type=float, default=4.0)
    ap.add_argument("--delight", type=float, default=0.7, help="0 = off, 1 = full")
    ap.add_argument("--delight-sigma", type=float, default=48.0)
    args = ap.parse_args()

    os.makedirs(args.out_dir, exist_ok=True)
    rgb = np.asarray(Image.open(args.albedo).convert("RGB"), dtype=np.float32) / 255.0
    lum = rgb @ np.array([0.299, 0.587, 0.114], dtype=np.float32)

    # De-light: divide out the large-scale brightness variation.
    low = blur(lum, args.delight_sigma)
    gain = (lum.mean() / np.maximum(low, 1e-3)) ** args.delight
    albedo = np.clip(rgb * gain[..., None], 0, 1)
    dl_lum = albedo @ np.array([0.299, 0.587, 0.114], dtype=np.float32)

    # Height: mid-frequency luminance, dark = low. Small blur removes pixel noise.
    height = norm01(blur(dl_lum, 1.0) - blur(dl_lum, 24.0) * 0.5)

    # Normals from the height gradient (wrap-around central differences).
    dx = (np.roll(height, -1, axis=1) - np.roll(height, 1, axis=1)) * 0.5
    dy = (np.roll(height, -1, axis=0) - np.roll(height, 1, axis=0)) * 0.5
    nx, ny, nz = -dx * args.normal_strength * 8, dy * args.normal_strength * 8, np.ones_like(dx)
    inv = 1.0 / np.sqrt(nx * nx + ny * ny + nz * nz)
    nx, ny, nz = nx * inv, ny * inv, nz * inv
    normal_gl = np.stack([nx * 0.5 + 0.5, ny * 0.5 + 0.5, nz * 0.5 + 0.5], axis=-1)
    normal_dx = normal_gl.copy()
    normal_dx[..., 1] = 1.0 - normal_dx[..., 1]

    # Roughness: matte overall, rougher in crevices, a little smoother on bright faces.
    rough = np.clip(0.9 - 0.3 * norm01(blur(dl_lum, 2.0)) - 0.1 * height + 0.1, 0.35, 1.0)

    # Cavity AO: height below its neighbourhood average is occluded.
    cavity = blur(height, 10.0) - height
    ao = np.clip(1.0 - cavity * 2.2, 0.0, 1.0)
    ao = 0.35 + 0.65 * ao

    save(albedo, os.path.join(args.out_dir, "albedo.png"))
    save(height, os.path.join(args.out_dir, "height.png"))
    save(normal_gl, os.path.join(args.out_dir, "normal_gl.png"))
    save(normal_dx, os.path.join(args.out_dir, "normal_dx.png"))
    save(rough, os.path.join(args.out_dir, "roughness.png"))
    save(ao, os.path.join(args.out_dir, "ao.png"))
    save(np.stack([ao, rough, np.zeros_like(ao)], axis=-1), os.path.join(args.out_dir, "orm.png"))
    print("wrote", args.out_dir)


if __name__ == "__main__":
    main()
