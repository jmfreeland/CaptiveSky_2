# Wind Arch bedrock candidate — 2026-10-16

The current Tideglass Game capture shows the Wind Arch as narrow columns of
rounded, repeated stones. That reads more like stacked pebbles than a place
shaped by wind and time. I authored a separate offline candidate that keeps the
walk-through opening but uses two slightly leaning, unequal monoliths, one broad
off-level capstone, sparse seam moss, and low talus stones.
It does not replace the existing WindArch asset or change the Island map.

Source: `Scripts/AssetPipeline/assets/windarch_bedrock.py`.

Local review export (gitignored):

- FBX: `Saved/CaptiveSky/ComfyBlender/WindArchBedrock/SM_WindArchBedrock_01.fbx`
- Preview: `Saved/CaptiveSky/ComfyBlender/WindArchBedrock/Preview.png`
- Manifest: `Saved/CaptiveSky/ComfyBlender/WindArchBedrock/manifest.json`

Validation used Blender 5.2.2 with `ASSET_CYCLES_DEVICE=CPU` for the bake and
`ASSET_PREVIEW_ENGINE=CYCLES_CPU`, `ASSET_PREVIEW_SAMPLES=16` for the re-import
preview. The exported FBX re-imported with one `UVMap`, two material slots,
three embedded box colliders, and 1,200 triangles; the pipeline printed
`PROBLEMS []`. Bounds are 719.2 × 179.5 × 529.2 cm. The lowest mesh point is
3.52 cm below the origin, a small intentional ground embed for the talus rather
than a perfect zero-ground contact. The opening remains unobstructed in the
preview.

`verify_asset.py` now accepts `ASSET_PREVIEW_ENGINE=CYCLES_CPU` and optional
`ASSET_PREVIEW_SAMPLES` for a software-only material preview; its default EEVEE
path is unchanged. This allowed the preview to be produced without adding GPU
load alongside the open Unreal editor.

**Not yet verified:** the FBX has not been imported into Unreal, checked against
the saved marker/proxy bounds, or compared in the fixed Tideglass Game view.
Do not place it on the Island until those checks pass. The old runtime
stone-course presentation remains the active version in the meantime.
