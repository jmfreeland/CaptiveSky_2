# Woodland bluebell instanced-material fix (2026-10-19)

## Finding and change

The 2026-10-19 Island PIE run reported that the three imported Woodland Bluebell
base materials lacked `MATUSAGE_INSTANCED_STATIC_MESHES`, so Unreal substituted
the default material for the instanced foliage. An editor-side audit confirmed
all three assets were base `Material` objects with two graph expressions each,
no unresolved material-function references, and the missing usage flag.

`Scripts/Enable-BluebellInstancedMaterials.py` enables that usage, recompiles
all three materials before saving any of them, then verifies the saved state.
The Unreal editor log records successful compilation and save for bloom, leaf,
and stem, with `enabled_after_save=True` for each material.

## Verification and limits

A subsequent standalone PIE session on 2026-10-19 ran for 220.1 real seconds
with zero model requests. Its foliage startup completed and the log contains no
new Bluebell instanced-material fallback warnings. The earlier Rhododendron
warnings remain; their graph has known unresolved material-function references
and was deliberately not modified. This run did not measure frame-time/FPS or
judge Bluebell close-up readability, so neither performance nor visual quality
is claimed as verified.

The Island map was not saved. The material assets are generated review content
under `Content/Generated/ComfyBlender/WoodlandBluebells_Review_20261012/`;
`Content/` is ignored by git, so the helper script is the tracked, repeatable
repair while the saved `.uasset` changes remain local to this project.

## Next steps

- Diagnose why Tideglass runtime scatter logged zero pollinator flower sites and
  zero marsh-marigold/iris clumps, despite eight deterministic candidate slots.
- Revisit the remaining Rhododendron fallback separately after fixing or
  replacing its unresolved material-function dependencies.
- Capture a controlled in-game Bluebell close-up and measure the 30-FPS p95
  safeguard before treating the visual or performance gates as passed.
