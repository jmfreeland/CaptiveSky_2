# Landscape wetness: how it reaches the screen (2026-09-30)

The island ground now darkens, loses roughness and pools water on flat ground as environment wetness rises.
This note records why earlier attempts looked inert and what the working path is.

## Why the stock wetness and the first wet-parent capture showed nothing

- `Landscape_0` renders each component from a **baked per-component `LandscapeMaterialInstanceConstant`** (outered to
  the proxy), not from the proxy's `LandscapeMaterial`. `SetMaterial` and MIDs on components never reach the screen.
  (This also means the subsystem's per-component wetness MIDs are invisible for the landscape; see
  `InitializeLandscapeMaterials`.)
- Reparenting the proxy's MIC (`MI_Island_Landscape`) only takes effect if the baked instances are recached:
  `UpdateAllComponentMaterialInstances()`, then `InitStaticPermutation()` on each baked MIC, then
  `MarkRenderStateDirty()` on the components, after shader compilation has finished. The 204634 capture pair in the
  audit doc swapped the parent without the recache, so both passes drew the old graph (mean diff 2.3/255).
- Driving wetness through a component MID is the same dead end. A **Material Parameter Collection** reaches every
  render proxy, so the wet graph reads `Wetness` from `/Game/Environment/MPC_IslandEnvironment`.
- The environment subsystem only exists in Game/PIE worlds, so the capture harness writes the collection itself
  (`HoldEnvironmentWetness`).

## What exists now

- `Scripts/Create-LandscapeWetMaterial.py` builds `/Game/Materials/M_Island_Textured_Wet` (a copy of the authored
  parent plus a wet stage) and `/Game/Materials/MI_Island_Landscape_Wet`. Authored assets are untouched; `Content/` is
  gitignored, so rebuild locally with the script (full editor, `-ExecCmds="py <path>"` or `-ExecutePythonScript`).
- `UIslandEnvironmentSubsystem::UseWetLandscapeGraph()` swaps the landscape's parent to the wet graph at world begin
  play and restores it in `Deinitialize`. **Editor and PIE only** (`SetParentEditorOnly` asserts otherwise). Standalone
  `-game` and packaged builds keep the authored graph unless the wet stage is authored into the map's landscape
  material, which is a Content change for the user to approve.
- `Island.Wetness [amount]` holds wetness at 0..1 (no argument returns to the weather).
- `Capture-Viewpoints.ps1 -CompareLandscapeWetness -LandscapeParent /Game/Materials/M_Island_Textured_Wet` renders
  dry and fully-wet passes.

## Measured

`TideglassGroundDetail`, noon: mean RGB dry [72.0, 56.9, 4.5] vs wet [39.7, 32.1, 10.0]. The wet pass shows
sky-reflecting pools on flat ground only; slopes stay dry. Low sun (17:00) and `WindArchOverlook` look plausible.
Puddle noise tiles every 900 cm (`PuddleTileCm`); tune `PuddleCoverage`, `PuddleSharpness`, `WetDarken`,
`PuddleDarken` on the wet parent.
