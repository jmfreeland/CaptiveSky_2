# Wind Arch art pass

## Intent

Replace the white rectangular WindArch blockout with a reusable, weather-worn stone landmark while preserving the walk-through route. The source asset is `Scripts/AssetPipeline/assets/windarch.py`; it is built with the existing local Blender/ComfyBlender pipeline and does not modify the Island map or Unreal `Content/`.

The design uses irregular stone piers, shoulders, a broad lintel and a keystone, broken footing stones, and restrained moss. Collision is authored separately as two piers plus one lintel so the center passage remains open.

## Evidence so far

- A read-only capture of the live Unreal editor on 2026-10-11 still shows the WindArch as a large white rectangular frame beside the inn. This confirms the art target is still present; no map changes were made.
- `Scripts/AssetPipeline/tests/test_windarch.py` passes both source-level geometry contract tests: no collision at the passage center at 1.8 m character height; at least 3.0 m passage width and 3.5 m overhead clearance. The current authored collision yields approximately 3.50 m width and 4.03 m overhead clearance.
- `lib.py` honors `ASSET_CYCLES_DEVICE=CPU` to allow an explicit CPU-only stone bake; the default remains `AUTO` for existing assets.
- Python AST parsing passes for the WindArch source and pipeline library.

## Local export and visual review (2026-10-11)

The asset pipeline was run with `ASSET_CYCLES_DEVICE=CPU`. The local FBX round-trip and texture pack passed: bounds are 7.059 × 1.972 × 5.217 m, with 2,080 triangles, two material slots, three box colliders, and a single `UVMap`; the verifier reported `PROBLEMS []` and `VERIFY DONE`. The authored collision leaves approximately 3.50 m clear width and 4.03 m clear height. The focused Wind Arch collision tests pass 2/2; the complete AssetPipeline suite passes 23/23.

The local [preview render](../../Saved/CaptiveSky/ComfyBlender/WindArch/Preview.png) reads as a broad walk-through landmark with the passage visible. The stone is pale and fairly uniform under the verifier's simple lighting; compare it under Island lighting before deciding whether to warm or darken the material. The pack/verify logs are under `Saved/CaptiveSky/ComfyBlender/_work/logs/` (`windarch_build.txt` and `windarch_verify.txt`). Blender emitted a nonfatal extension-cache `WinError 183` and shutdown `Unable to delete file` messages while an older Blender process (PID 18040) remained open; despite these warnings, export, texture packing, re-import, and verifier all completed. Do not terminate that pre-existing process based on these logs.

## Next steps

1. When Unreal is available, import only the asset into `/Game/Generated/ComfyBlender/WindArch/` and inspect material, scale, and collision under real Island lighting; do not place it yet.
2. If the in-world material reads too pale, adjust only the Wind Arch asset/material and re-review; leave the shared source library untouched.
3. Request approval before replacing or moving the map's existing blockout.
4. Re-run `Scripts/AssetPipeline/tests/test_windarch.py` after any collision adjustment.
