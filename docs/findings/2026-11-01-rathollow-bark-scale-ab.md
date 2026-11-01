# RatHollow bark-scale comparison: V9 / V10 / V11

## Result

Keep V9's 1.2 m bark-texture tile scale as the current offline preference. V10's
1.8 m scale makes the bark plates read too coarse; V11's 0.8 m scale makes the
longitudinal grooves busier and more fibrous. These are controlled material-UV
comparisons, not final art approval. None has been imported into Unreal or placed
in the Island.

## Controlled change

All three assets use the same V9 geometry, material images, camera, light, and
CPU render settings. Only the trunk shell's bark UV tile size changes:

| Candidate | Bark tile | Build size | Triangles | Colliders | Result |
| --- | ---: | --- | ---: | ---: | --- |
| V9 | 1.2 m | 263 × 146 × 142 cm | 2,510 | 5 box | Best balance of readable bark and restrained scale |
| V10 | 1.8 m | 263 × 146 × 142 cm | 2,510 | 5 box | Plates are oversized and coarse |
| V11 | 0.8 m | 263 × 146 × 142 cm | 2,510 | 5 box | More numerous, busier longitudinal grooves |

Every re-import verifier reported `PROBLEMS []`, the same five colliders, the same
two named anchors, and one UV layer. All previews rendered with Blender 5.2.2
Cycles on CPU at 8 samples; no GPU render, ComfyUI run, Unreal import, map save,
or character-home assignment was performed. Exported preview files are local and
gitignored under `Saved/CompileScratch/Codex_RatHollowTextureScaleAB_20261101/`.

The broader remaining art issue is the source texture itself: its bark plates are
strongly directional, so UV scale alone cannot make the surface read like naturally
weathered fallen wood. A new, less repetitive bark texture or carefully layered
color/roughness variation is a more promising next art pass than another scale
adjustment. Validate any texture change under Island lighting and at Rat gameplay
scale before placement.
