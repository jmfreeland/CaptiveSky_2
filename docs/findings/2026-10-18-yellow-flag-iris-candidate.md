# Yellow-flag iris wetland candidate

## Intent

Add a distinctive plant form for quiet freshwater margins: a broad-leaved iris rather than another fine grass or low shoreline mat. The candidate is procedural and deterministic so the source remains editable and reproducible. It is not based on a downloaded or licensed mesh.

## Current artifact

- Source: `Scripts/AssetPipeline/assets/wetland_yellow_flag_iris.py`
- Local export: `Saved/CaptiveSky/ComfyBlender/WetlandYellowFlagIris/SM_WetlandYellowFlagIris_01.fbx` (gitignored)
- Local preview: `Saved/CaptiveSky/ComfyBlender/WetlandYellowFlagIris/Preview.png` (gitignored; CPU render)
- Geometry: 10 sword leaves arranged as three fans, 3 flower stalks, 4 blooms; approximately 32.7 × 36.7 × 85.5 cm and 7,500 triangles. Amber signal marks sit on the visible upper side of each drooping fall.
- Materials: five simple leaf, midrib, stalk, petal, and amber-signal slots; no baked or generated textures.
- Collision: none, intentionally, so test placement cannot add blocking geometry at the shore.

## Validation

On 2026-10-18 with Blender 5.2.2 LTS:

- The initial source-contract suite passed (3 tests); after adding refinement regressions, all 4 tests passed.
- `run_asset.py` exported the FBX successfully with the expected dimensions, materials, and 7,500 triangle count.
- `verify_asset.py` re-imported the FBX and reported `UVMap`, five expected slots, no collision meshes, `ZMIN 0.0117`, `PROBLEMS []`, and `VERIFY DONE`. The preview render used one Cycles CPU sample because the normal Eevee preview exceeded the available command window while the Unreal editor was open.
- Visual inspection of the refreshed preview shows three broader leaf fans and contrasting amber marks on the yellow blooms. This remains a silhouette/palette sanity check only; it does not validate Unreal material response, scale in the environment, performance, composition, or plant placement.

## Next review gate

The generic static-mesh preview is complete. Next, inspect the imported mesh under representative Island daylight without saving or changing the Island map; verify material colors, reverse-angle appearance, and whether the blooms read clearly at normal viewing distance. Only after that review should any placement be proposed. Keep it out of the saved landscape until it demonstrably improves the wetland's visual variety without worsening frame time or texture pressure.

## Unreal import-only review (2026-10-18)

- With the existing `CaptiveSky_2` editor responsive, imported into a fresh isolated review folder: `/Game/Generated/ComfyBlender/WetlandYellowFlagIris_Review_20261018/`. No Island actor was added and the Island map was not saved.
- Importer report: `Saved/CaptiveSky/ComfyBlender/WetlandYellowFlagIris/unreal_import.json`. Unreal measured 32.7 × 36.7 × 85.5 cm against the source's 33 × 37 × 85 cm; all five material slots were assigned, there were no texture imports or collision shapes, and `validation_errors` is empty.
- Unreal's FBX Message Log reported that the scene had no smoothing-group information. The import completed; inspect the normals in the static-mesh preview before accepting the asset visually.
- The static-mesh editor opened the imported mesh successfully. Its viewport reports 7,500 triangles, 5,990 vertices, two UV channels, zero collision primitives, and 0.12 MB estimated compressed mesh size. The five material swatches and mesh render with the expected green blades and yellow/amber blooms; no missing faces or severe normal artifacts are apparent from the inspected angle. The source builds closed, thin blade geometry rather than flat cards, so the one-sided leaf material is not by itself evidence of a backface failure.
- This validates the asset as a lightweight, distinct broad-leaf silhouette, not the scene placement: the editor uses its generic preview floor/environment, flowers remain small at thumbnail scale, and the mesh has only one LOD. Still pending: inspect the reverse angle and appearance at Island wetland daylight, then a small, bounded placement/frame-time check before any broad scatter.
