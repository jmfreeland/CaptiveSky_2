# Woodland foxglove candidate (2026-10-13)

Added `Scripts/AssetPipeline/assets/woodland_foxglove.py` as a scripted, isolated
woodland-edge species candidate. The deterministic cluster has three upright
flower spires, eight pendant bells per spire, and five broad basal leaves per
plant. Its four plain material slots separate leaf, stem, bloom, and throat
colors; it intentionally adds no character collision.

## Export review

Built with Blender 5.2.2 to the ignored local review folder
`Saved/CaptiveSky/ComfyBlender/Codex_WoodlandFoxglove_20261013/WoodlandFoxglove/`.
The re-import verifier reports dimensions of 1.405 x 1.211 x 1.293 m, 7,788
triangles, one `UVMap`, four material slots, no collision meshes, ground minimum
Z=-0.003 m, and `PROBLEMS []`. The preview is rendered from the exported FBX and
its material manifest, not the Blender source scene:

![Woodland foxglove exported-FBX preview](../../Saved/CaptiveSky/ComfyBlender/Codex_WoodlandFoxglove_20261013/WoodlandFoxglove/Preview.png)

The silhouette reads as a compact three-spire colony, with enough height to
break up low woodland-edge ground cover. In this preview the leaves are strongly
faceted and the flowers read as simple lavender bells; throat detail is not
prominent at this camera distance. Review in an Island context before deciding
whether to soften the leaf planes, tune the flower color, or use the candidate.

All 31 tests in `Scripts/AssetPipeline/tests` passed, including four source
contracts for deterministic composition, the separated material palette,
flower detail helpers, and collision-free ground cover.

## Scope and next step

This is a local asset-pipeline candidate only. It has not been imported into
Unreal, assigned a foliage/PCG placement rule, or placed on the Island. The
latest recorded project startup attempt in `Saved/Crashes/_0000/` is a null
access violation at 2026-10-13 13:12; no Unreal Editor process was running during
this review. Keep the no-retry gate until startup can be diagnosed, then inspect
scale, shading, silhouette, and frame cost in a representative Island location
before approving broad placement.
