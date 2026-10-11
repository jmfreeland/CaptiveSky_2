"""Generate a small, deterministic, tileable baked-crust PBR texture set."""
import argparse
import math
from pathlib import Path
import sys

import numpy as np
from PIL import Image

PIPE = Path(__file__).resolve().parent
sys.path.insert(0, str(PIPE))
from config import ROOT  # noqa: E402


def periodic_fbm(size, seed, octaves=7, waves_per_octave=8):
    """Smooth multi-scale field built only from integer-frequency harmonics."""
    octaves = min(octaves, max(2, int(math.log2(size)) - 2))
    rng = np.random.default_rng(seed)
    axis = np.arange(size, dtype=np.float32) / float(size)
    x, y = np.meshgrid(axis, axis)
    field = np.zeros((size, size), dtype=np.float32)
    weight_sum = 0.0
    for octave in range(octaves):
        band = 2 ** octave
        weight = 1.0 / (1.65 ** octave)
        for _ in range(waves_per_octave):
            magnitude = float(rng.uniform(band, max(band + 1, band * 2)))
            angle = float(rng.uniform(0.0, 2.0 * math.pi))
            kx = int(round(magnitude * math.cos(angle)))
            ky = int(round(magnitude * math.sin(angle)))
            if kx == 0 and ky == 0:
                kx = 1
            phase = float(rng.uniform(0.0, 2.0 * math.pi))
            angle = 2.0 * math.pi * (kx * x + ky * y) + phase
            field += np.cos(angle).astype(np.float32) * weight
        weight_sum += waves_per_octave * weight
    field /= max(weight_sum, 1e-6)
    field -= float(field.mean())
    field /= max(float(field.std()), 1e-6)
    return field


def make_maps(size=512, seed=20261011):
    """Return sRGB albedo, DirectX normal, and Unreal ORM arrays."""
    if size < 64 or size > 2048 or size & (size - 1):
        raise ValueError("size must be a power of two from 64 through 2048")

    broad = periodic_fbm(size, seed, octaves=6)
    grain = periodic_fbm(size, seed + 1, octaves=8)
    fine = periodic_fbm(size, seed + 2, octaves=9)

    # The palette drifts between warm wheat and darker toasted amber rather
    # than terracotta. Integer Fourier frequencies keep every scale periodic.
    toast = np.clip(0.44 + 0.17 * broad + 0.07 * grain + 0.035 * fine, 0.0, 1.0)
    toast = toast * toast * (3.0 - 2.0 * toast)
    light = np.asarray((204.0, 153.0, 91.0), dtype=np.float32)
    dark = np.asarray((128.0, 72.0, 37.0), dtype=np.float32)
    rgb = light[None, None, :] * (1.0 - 0.66 * toast[:, :, None])
    rgb += dark[None, None, :] * (0.66 * toast[:, :, None])
    rgb += fine[:, :, None] * np.asarray((5.0, 4.0, 2.5), dtype=np.float32)

    height = np.clip(0.53 + 0.075 * broad + 0.035 * grain + 0.014 * fine, 0.0, 1.0)
    ao = np.full((size, size), 0.99, dtype=np.float32)
    rng = np.random.default_rng(seed + 3)
    # Sparse pinholes and shallow blisters, wrapped over tile boundaries.
    for _ in range(max(48, size // 2)):
        cx = int(rng.integers(0, size))
        cy = int(rng.integers(0, size))
        radius = int(rng.integers(1, 4))
        for dy in range(-radius, radius + 1):
            for dx in range(-radius, radius + 1):
                if dx * dx + dy * dy > radius * radius:
                    continue
                px, py = (cx + dx) % size, (cy + dy) % size
                depth = 1.0 - math.sqrt(dx * dx + dy * dy) / max(radius, 1)
                rgb[py, px] *= 1.0 - 0.42 * depth
                height[py, px] -= 0.035 * depth
                ao[py, px] -= 0.16 * depth

    # Derive a restrained, periodic tangent normal from the height field.
    dx = (np.roll(height, -1, axis=1) - np.roll(height, 1, axis=1)) * 0.5
    dy = (np.roll(height, -1, axis=0) - np.roll(height, 1, axis=0)) * 0.5
    normal = np.stack((-dx * 46.0, dy * 46.0, np.ones_like(height)), axis=-1)
    normal /= np.maximum(np.linalg.norm(normal, axis=-1, keepdims=True), 1e-6)
    normal_dx = np.clip(normal * np.asarray((0.5, -0.5, 0.5), dtype=np.float32) + 0.5, 0.0, 1.0)

    roughness = np.clip(0.84 + 0.055 * grain + 0.025 * fine, 0.68, 0.98)
    orm = np.stack((ao, roughness, np.zeros_like(ao)), axis=-1)

    return (
        np.clip(rgb, 0.0, 255.0).astype(np.uint8),
        (normal_dx * 255.0 + 0.5).astype(np.uint8),
        (np.clip(orm, 0.0, 1.0) * 255.0 + 0.5).astype(np.uint8),
    )


def generate(out_dir=None, size=512, seed=20261011):
    out_dir = Path(out_dir or (Path(ROOT) / "Library" / "Textures"))
    out_dir.mkdir(parents=True, exist_ok=True)
    albedo, normal, orm = make_maps(size, seed)
    paths = {
        "BC": out_dir / "T_Lib_Bread_BC.png",
        "N": out_dir / "T_Lib_Bread_N.png",
        "ORM": out_dir / "T_Lib_Bread_ORM.png",
    }
    for key, array in (("BC", albedo), ("N", normal), ("ORM", orm)):
        Image.fromarray(array, mode="RGB").save(paths[key], optimize=True)
    return paths


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--size", type=int, default=512)
    parser.add_argument("--seed", type=int, default=20261011)
    parser.add_argument("--out-dir", default=None)
    args = parser.parse_args()
    for key, path in generate(args.out_dir, args.size, args.seed).items():
        print("{}: {}".format(key, path))


if __name__ == "__main__":
    main()
