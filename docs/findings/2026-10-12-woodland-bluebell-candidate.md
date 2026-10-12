# Woodland bluebell clump candidate

**State:** isolated asset candidate; imported for review, not placed in the Island.

Added `Scripts/AssetPipeline/assets/woodland_bluebells.py` as a deterministic,
script-built flowering ground-cover species. The clump has seven stems, low
strap leaves, drooping lavender bells, three parameter-only material slots,
and no collision. It is intended as a shaded woodland accent that can later
be distributed in restrained PCG/HISM patches rather than used as a large
landscape carpet.

## Validation (2026-10-12)

- Blender 5.2.2 exported the FBX and re-imported it for the preview. The
  measured bounds are 62.7 x 50.6 x 36.0 cm; origin is at ground level
  (`ZMIN 0.0054 m`), there is one `UVMap`, three material slots, 5,116
  triangles, and `PROBLEMS []`.
- Unreal 5.8.3 commandlet imported it to the fresh review folder
  `/Game/Generated/ComfyBlender/WoodlandBluebells_Review_20261012` with
  `validation_errors: []`, matching dimensions and all three material
  assignments, and zero collision shapes. It ran with `NullRHI`; no map was
  opened or saved.
- Evidence: local-only
  `Saved/CaptiveSky/ComfyBlender/WoodlandBluebells/Preview.png`,
  `unreal_import.json`, and
  `Saved/Logs/Codex_WoodlandBluebellsImport_20261012.log`.

## Remaining review

The FBX preview is a deliberately stylized low-poly interpretation, not a
photoreal botanical asset. Its 5.1k triangles per clump are high for dense
ground cover; if approved for placement, test a lower-poly variant or keep
patch density sparse. Real-RHI appearance, translucency/two-sided foliage
settings, wind response, LODs, PCG distribution, and gameplay frame-time are
not validated. Review it beside the existing fern in a shaded Island
viewpoint before deciding whether it belongs in the landscape.
