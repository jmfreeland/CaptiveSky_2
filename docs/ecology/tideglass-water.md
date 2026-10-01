# Tideglass water

The first rendered Tideglass view showed a nearly featureless white flattened sphere. A preview of
the existing ocean material on that shallow mesh rendered almost black, so it is reserved for the
open ocean. At Game/PIE start the subsystem now uses the saved sphere only as a reversible footprint:
it creates a 64-segment procedural surface with two shallow rings and a softened irregular outline,
then hides the sphere until teardown. The generated top has no collision; the original footprint's
collision remains in place. It uses a separate opaque teal material with two pool-scale world-aligned
animated normal swells, a soft edge reflection, and weather-driven color, roughness and normal strength from
`MPC_IslandEnvironment` (`WindSpeed`, `Storm`, and `RainIntensity`). The Island map and blockout
component are never saved with runtime changes. This is a visual prototype, not a depth, temperature,
water-quality, or swimming simulation.

`Scripts/Create-TideglassPoolMaterial.py` creates `/Game/Materials/M_TideglassPool_Lively` and refuses
to overwrite that output. Both older `/Game/Materials/M_TideglassPool` and
`/Game/Materials/M_TideglassPool_NormalizedWind` assets are preserved; the generated `.uasset` is in
the ignored `Content/` tree and is not versioned here. Run the script headlessly with UE 5.8.3
`-ExecutePythonScript` when the new asset is missing.
`UIslandTideglassSubsystem` finds the flattened sphere beside the `TideglassPool` marker and generates
the runtime-only surface over its bounds. It restores the sphere's prior visibility and destroys the
procedural component at world teardown. Without the generated material asset, it leaves the original
white placeholder visible.

For an editor-world-only rendered comparison, run:

```powershell
./Scripts/Capture-Viewpoints.ps1 -Hour 12 -Only Tideglass -NoWorldState -LogPath Saved/Logs/TideglassBaseline.log
./Scripts/Capture-Viewpoints.ps1 -Hour 12 -Only Tideglass -TideglassMaterial /Game/Materials/M_TideglassPool_Lively -NoWorldState -LogPath Saved/Logs/TideglassTeal.log
./Scripts/Capture-Viewpoints.ps1 -Hour 12 -Only Tideglass -TideglassMaterial /Game/Materials/M_TideglassPool_Lively -TideglassWeather Calm -NoWorldState
./Scripts/Capture-Viewpoints.ps1 -Hour 12 -Only Tideglass -TideglassMaterial /Game/Materials/M_TideglassPool_Lively -TideglassWeather Storm -NoWorldState
```

The material preview uses the same transient procedural geometry and restores the blockout's original
visibility after rendering. A first procedural capture omitted the pool. The render-state probe showed
the section was registered, correctly placed and above the landscape; its top-face winding was reversed
for Unreal's renderer. Correcting the winding made the surface visible, and the automation now guards
that index order. The October 1 golden-hour image at
`Saved/Viewpoints/2026-10-01_090523_h17.0/02_Tideglass.png` shows the current material and organic
outline together. For `/Game/Materials/M_TideglassPool_Lively`, matched noon captures are in
`Saved/Viewpoints/2026-10-01_085528_h12.0/` (calm) and
`Saved/Viewpoints/2026-10-01_085616_h12.0/` (storm), with forced collection values restored afterward.
The smaller pool-scale normal tiling produces visible surface breakup; the storm frame is rougher and
more agitated than calm. The rejected `M_IslandOcean` preview is recorded in
`Saved/Logs/Codex_TideglassWaterPreview_MIslandOcean_20261001.log`; the original baseline and teal
captures remain under `Saved/Viewpoints/2026-10-01_065235_h12.0/` and
`Saved/Viewpoints/2026-10-01_070300_h12.0/`.

UE 5.8.3 validation completed:

- The UE 5.8.3 editor build and `CaptiveSky2.Agent.IslandTideglass` passed. The test checks the
  real saved-map target finder plus synthetic Game-world application, default-asset loading,
  transform/topology/material, render-facing winding, no-collision behavior, and
  restoration/destruction at teardown.
- `CaptiveSky2.Visual.Viewpoints` passed with the same transient geometry at noon and golden hour;
  the rendered image, not the test result alone, confirmed the outline is visible.
- A provider-free Game probe applied the runtime material, completed the Innkeeper's 959 cm route,
  and ended after 11.3 real seconds with zero model requests. It used
  `Saved/Playtests/Codex_TideglassRuntime_20261001` for isolated data.

`-TideglassWeather Calm` or `-TideglassWeather Storm` holds the actual `MPC_IslandEnvironment`
wind/storm/rain scalars at known diagnostic values for the render, then restores their captured
values. This tests the material response, not the weather actor or gameplay integration. Wind is
normalized against a tunable 300 cm/s reference before blending, avoiding immediate saturation at
ordinary breeze speeds. The new material's waves now span the small pool footprint, and its matched
storm frame visibly roughens the surface. These editor-world stills verify weather-driven variation,
not temporal motion during play. A bounded provider-free Game capture is now reproducible with:

```powershell
./Scripts/Start-Spectator.ps1 -Windowed -Shots -DisableAgentThinking -DataRoot Saved/Playtests/TideglassMotion -MaxRealtimeSeconds 40 -MaxModelRequests 1 -ScreenshotDirectory Playtests/TideglassMotion/Screenshots -ViewpointFile Config/TideglassMotionProbe.json -EstablishingSeconds 10
```

The October 1 run completed in 40.2 seconds, made zero model requests, and produced four 1600x900
frames under `Saved/Playtests/TideglassMotion/Screenshots/`. The visible pool pattern differs between
the first and last frames, but the environment clock advanced from 09:00 to 09:18 and the surrounding
lighting changed too; this is suggestive, not a clean isolation of animated swells. Freeze the clock
or compare material-space motion under fixed lighting before calling temporal motion confirmed. That
Game launch also logged nine Python tracebacks from UE's Experimental Toolsets/ToolsetRegistry startup
scripts (missing editor Python classes); no CaptiveSky error appeared, and the game exited normally.
The organic surface remains a non-colliding visual prototype without depth,
temperature, water quality, swimming or shoreline blending. Keep the map material reversible and do
not overwrite the authored sphere material.

## Runtime lifecycle follow-up (2026-10-01)

The `CaptiveSky2.Agent.IslandTideglass` regression starts its synthetic Game world with the normal
`UWorld::BeginPlay()` lifecycle instead of manually invoking
`UIslandTideglassSubsystem::OnWorldBeginPlay()`. When the generated asset is present, the test uses
the same default material path as Game startup; otherwise it injects a transient fallback. It
asserts the procedural surface is created, receives the startup material, and leaves the hidden map
sphere's authored material intact. The UE 5.8.3 test passed using the actual default Lively asset.
Startup also logs whether the material is missing or the tagged flattened footprint cannot be applied,
while leaving maps without a Tideglass marker quiet.

A bounded real-RHI Game capture with the current source confirmed that the default water material
applies at world start and the Tideglass ground-cover system reports a 306 cm pool-edge clearance.
It ended normally at 60.1 seconds with zero model requests. The runtime capture at
`Saved/Playtests/Codex_TideglassRuntimeVisual_Elevated_20261001/Screenshots/001_Tideglass.png`
shows the pool in its intended cyan rather than as the white blockout disc; the paired overhead
capture at
`Saved/Playtests/Codex_TideglassOverhead_Elevated_20261001/Screenshots/001_Overhead.png`
shows the grass-free water footprint and the surrounding verge. Low-angle views still show grass
silhouettes crossing the water in screen space; the overhead view indicates that is foreground
occlusion, not plants rooted inside the pool. Both runs used isolated world-state roots and restored
the original editor DLL byte-for-byte. The Game log still contains the known UE Experimental Toolsets
Python startup tracebacks, but the run exited normally. These are capped spectator checks, not a
packaged-performance benchmark or autonomous resident behavior test.

Logs: `Saved/Logs/Codex_TideglassLifecycle_Elevated_20261001.log`,
`Saved/Logs/Codex_TideglassRuntimeVisual_Elevated_20261001.log`, and
`Saved/Logs/Codex_TideglassOverhead_Elevated_20261001.log`.
