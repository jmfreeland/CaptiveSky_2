"""Procedural rat fur BaseColor (no GPU): warm grey-brown with fine streaks and low-frequency tonal variation.
    python rat_fur_texture.py <out.png>
"""
import sys
import numpy as np
from PIL import Image

N = 1024
rng = np.random.default_rng(7)


def tile_noise(scale):
    g = rng.random((scale, scale))
    img = Image.fromarray((g * 255).astype(np.uint8)).resize((N, N), Image.BICUBIC)
    return np.asarray(img, dtype=np.float32) / 255.0


low = tile_noise(6) * 0.6 + tile_noise(14) * 0.4
streak = np.asarray(Image.fromarray((rng.random((N // 2, 24)) * 255).astype(np.uint8)).resize((N, N), Image.BICUBIC), dtype=np.float32) / 255.0
fine = rng.random((N, N)).astype(np.float32)
v = 0.50 * low + 0.35 * streak + 0.15 * fine
base = np.array([0.155, 0.13, 0.11], dtype=np.float32)            # dark warm brown-grey
dark = np.array([0.065, 0.055, 0.05], dtype=np.float32)
img = dark + (base - dark) * v[..., None] * 1.25
img = np.clip(img, 0, 1)
Image.fromarray((img ** (1 / 2.2) * 255).astype(np.uint8)).save(sys.argv[1])
print("wrote", sys.argv[1])
