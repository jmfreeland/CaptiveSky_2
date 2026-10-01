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
The original puddle-noise scale was 900 cm (`PuddleTileCm`); the 2026-10-01 controlled comparison
promoted a broader 1,200 cm / 0.36 coverage / 6.5 sharpness setting into the builder defaults.
`WetDarken` and `PuddleDarken` remain unchanged.

## Pool-scale comparison and selected builder defaults (2026-10-01)

To reduce small reflective flecks in the wide island shots without losing readable wet ground near
Tideglass, `Scripts/Create-LandscapeWetMaterialPrototype.py` generated three separate variants for
comparison against the original builder defaults.
Each variant was captured dry and fully wet in the same UE 5.8.3 automation run at noon, with no
world-state writes. The commandlet used the scratch `.uproject`, but its `Content/` directory is a
junction to the main project's `Content/`; therefore the builder also regenerated the ignored
`M_Island_Textured_Wet` and `MI_Island_Landscape_Wet` assets in the main Content tree. The saved
landscape assignment, authored material, and runtime weather/world state were not changed.

| Variant | `PuddleTileCm` | `PuddleCoverage` | `PuddleSharpness` |
|---|---:|---:|---:|
| Original | 900 | 0.40 | 8.0 |
| BroadPools | 1,800 | 0.28 | 5.0 |
| BalancedPools | 1,300 | 0.34 | 6.0 |
| GentlePools | 1,200 | 0.36 | 6.5 |

The table below reports the share of pixels in the lower two-thirds of each image whose blue
channel increased by more than 20 levels between its paired dry and wet captures. This is a quick
reflection-change proxy, not the physical puddle coverage of the material; view angle, roughness,
lighting and surface orientation all affect it.

| Viewpoint | Original | BroadPools | BalancedPools | GentlePools |
|---|---:|---:|---:|---:|
| ShoreApproach | 2.37% | 0.35% | 0.95% | 1.39% |
| Tideglass | 5.53% | 0.78% | 2.76% | 2.67% |
| TideglassGroundDetail | 2.88% | 0.00% | 0.97% | 8.36% |
| ListeningStones | 6.35% | 1.34% | 2.78% | 3.44% |
| WindArchOverlook | 3.69% | 0.62% | 1.72% | 2.44% |

BroadPools removes too much wet response. BalancedPools suppresses the wide-shot flecks but makes
the close ground response weak. GentlePools gives the clearest connected sky reflections in the
close ground detail while cutting the reflection proxy in four of five wide views versus the
original. Accordingly, `Scripts/Create-LandscapeWetMaterial.py` now builds that measured parameter
set by default; all other wet response values and the landscape graph wiring are unchanged.

UE's material builder completed successfully and created both generated wet assets. A fresh nine-view dry/wet automation pair against the builder output passed, prepared all
4,096 landscape component slots transiently, and restored the authored parent afterward. The
builder and capture logs are `Saved/CompileScratch/AgentMovementAutomationProjectWithContent/Saved/Logs/Codex_BuildGentleWetMaterial_20261001.log`
and `.../Codex_WetnessBuilderDefaultPair_20261001.log`; the captures are under
`Saved/CompileScratch/AgentMovementAutomationProjectWithContent/Saved/Viewpoints/2026-10-01_010810_h12.0_{authored_dry,fully_wet}/`.
The automation logs include expected offline Epic-service connection warnings; the UE processes
exited 0 and no material compile error or crash was recorded.

### Mid-wetness visual comparison (2026-10-01)

The full-wet comparison still showed a few bright reflective flecks in the island-wide camera, so
I rendered two more matched UE 5.8.3 previews with the environment collection held at a fixed
`Ground Wetness` of 0.65: `BalancedPools` and `GentlePools`. Both
`CaptiveSky2.Visual.Viewpoints` runs passed. They used the same noon camera and transient baked-parent
swap; neither changed the saved landscape assignment nor wrote world state.

At this partial wetness, `BalancedPools` keeps the wide view very quiet but its Tideglass ground
detail is almost indistinguishable from dry. `GentlePools` retains a few clearly readable close
reflections while the wide view stays restrained, with only a small number of glints remaining.
The comparison supports keeping `GentlePools` as the generated wet graph's default; it does not
justify making the graph the authored map material. Captures:
`Saved/CompileScratch/AgentMovementAutomationProjectWithContent/Saved/Viewpoints/2026-10-01_024510_h12.0/`
(`BalancedPools`) and `.../2026-10-01_024635_h12.0/` (`GentlePools`); logs:
`Saved/CompileScratch/AgentMovementAutomationProjectWithContent/Saved/Logs/Codex_WetPoolsPartial65_BalancedPools_20261001.log`
and `.../Codex_WetPoolsPartial65_GentlePools_20261001.log`.

This is a scalar preview, not a rain-driven PIE transition. Confirm the dry → wet → drying sequence
in PIE before changing the Island's landscape assignment; standalone and packaged behavior remain
unverified.

This is still an editor/PIE graph: the current subsystem swaps it only in editor and PIE. The next
visual check should be a short PIE weather transition on the user's UE display, including authored
dry, rain rising, and drying down. Do not treat these offscreen captures as confirmation of packaged
or standalone rendering.

## Authored into the map's landscape material (2026-10-01)

At the user's request, `/Game/Materials/MI_Island_Landscape` is now reparented onto `M_Island_Textured_Wet`, so
standalone and packaged builds get the wet response without the editor-only swap (`UseWetLandscapeGraph` sees the
parent is already the wet graph and does nothing). The previous instance is kept as
`/Game/Materials/MI_Island_Landscape_AuthoredBackup`; `Content/` is gitignored, so that copy is the only way back.

- Build or rebuild: `LANDSCAPE_WET_AUTHOR=1` with `Create-LandscapeWetMaterial.py` (full editor, `-ExecutePythonScript`).
  Later runs without the variable keep the authored state (the script detaches the instance while it rebuilds the
  wet parent). `LANDSCAPE_WET_AUTHOR=0` restores the original parent.
- Wetness 0 is the old look: the `TideglassGroundDetail` authored-dry pass measured the same mean RGB as before
  authoring ([72.00, 56.86, 4.54]); fully wet [39.5, 33.0, 12.3] with no `-LandscapeParent` swap.
- Standalone check: `-game /Game/Maps/Island -Spectator -SpectatorShots` with `-ExecCmds="Island.Wetness 1"` renders dark,
  glossy ground with pools around the inn, with no editor involved.
- Not yet checked: a rain-driven dry -> wet -> drying sequence in PIE or a packaged build.
