# Wind Arch art pass

## Intent

Replace the white rectangular WindArch blockout with a reusable, weather-worn stone landmark while preserving the walk-through route. The source asset is `Scripts/AssetPipeline/assets/windarch.py`; it is built with the existing local Blender/ComfyBlender pipeline and does not modify the Island map or Unreal `Content/`.

The design uses irregular stone piers, shoulders, a broad lintel and a keystone, broken footing stones, and restrained moss. Collision is authored separately as two piers plus one lintel so the center passage remains open.

## Evidence so far

- A read-only capture of the live Unreal editor on 2026-10-11 still shows the WindArch as a large white rectangular frame beside the inn. This confirms the art target is still present; no map changes were made.
- `Scripts/AssetPipeline/tests/test_windarch.py` passes both source-level geometry contract tests: no collision at the passage center at 1.8 m character height; at least 3.0 m passage width and 3.5 m overhead clearance. The current authored collision yields approximately 3.50 m width and 4.03 m overhead clearance.
- `lib.py` honors `ASSET_CYCLES_DEVICE=CPU` to allow an explicit CPU-only stone bake; the default remains `AUTO` for existing assets.
- Python AST parsing passes for the WindArch source and pipeline library.

## Not yet verified

The FBX, packed stone textures, Blender re-import render, and Unreal import have not been generated or reviewed. On 2026-10-11 the live Unreal editor was responsive at roughly 27 GB private memory; a Blender process (PID 18040) had also been running since 2026-10-10. Commit usage was about 60.7 of 65.8 GB. A second Blender process was not launched to avoid competing for memory/GPU while the user/editor work may be active.

## Next steps

1. When the existing editor and Blender sessions are no longer needed, identify whether the existing Blender process has an active task before closing anything.
2. Run the asset pipeline for `windarch` with `ASSET_CYCLES_DEVICE=CPU`; inspect the build log, packed textures, manifest bounds/triangle count, and re-import `Preview.png`.
3. If the exported preview is sound, import only the asset into `/Game/Generated/ComfyBlender/WindArch/`, inspect scale/material/collision in Unreal, and request approval before replacing or moving the map's blockout.
4. Re-run `Scripts/AssetPipeline/tests/test_windarch.py` after any collision adjustment.
