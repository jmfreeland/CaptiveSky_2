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

When Unreal is responsive, import into a fresh isolated `/Game/Generated/ComfyBlender/WetlandYellowFlagIris/` asset folder without touching the Island map. Inspect at player scale beside the waterline, under the landscape material and representative daylight; verify material colors, leaf-sidedness, and whether the blooms read clearly at normal viewing distance. Only after that review should any placement be proposed. Keep it out of the saved landscape until it demonstrably improves the wetland's visual variety without worsening frame time or texture pressure.
