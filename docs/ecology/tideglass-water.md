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

## Shore-stone presentation (2026-10-05; Game verification 2026-10-06)

The four small pale spheres around the Tideglass pool were not water highlights: a read-only saved-map
actor audit identified them as `TideglassPool_Stone_0..3`, static-mesh actors using
`/Engine/BasicShapes/Sphere.Sphere` and the default Basic Shapes material. At Game/PIE start,
`UIslandTideglassSubsystem` now adds transient `/Game/StarterContent/Props/SM_Rock` visuals at their
existing bounds and hides the four placeholder meshes in game. Their saved transforms, collision,
navigation role, and map data remain the authority; the presentation actors have no collision and are
destroyed at teardown, when each proxy's previous hidden state is restored. If the complete four-proxy
set or Starter Content rock mesh is unavailable, the subsystem leaves the saved placeholders alone.

The first implementation matched the editor labels as runtime object names. Automation passed because
its synthetic actors used those same explicit names, but the saved Island instantiates them in Game as
`StaticMeshActor_11..14`; the four pale spheres therefore remained visible. A runtime scan confirmed
that the pool itself is a much larger sphere and the four proxies have world-space bounds extents of
22.5 x 22.5 x 17.5 cm. Selection now uses the exact engine sphere mesh and a maximum world-space bound
extent of 25 cm within 500 cm of the Tideglass marker, avoiding the editor-label/runtime-name mismatch
and excluding the flattened pool surface. The regression test now leaves object names generated, like
the real Game world.

UE 5.8.3 validation: the editor target built, `CaptiveSky2.Agent.IslandTideglass` passed with generated
object names, all four rough-rock visuals, proxy collision preservation, collision-free visuals, and
teardown restoration/removal. A bounded real Game capture then logged that all four visuals were
applied and showed the pool without the pale spheres. Screenshot:
`Playtests/Codex_TideglassRocksFixed_20261006/Screenshots/002_Tideglass.png`. Runtime evidence is in
`Saved/Logs/Codex_TideglassScan_20261006.log` and
`Saved/Logs/Codex_TideglassRocksFixed_20261006.log`; the automation result is in
`Saved/Logs/Codex_TideglassRuntimeNames_20261006.log`.

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

## Weather-glint art-direction pass (2026-10-05)

The 2026-10-05 in-game Tideglass frame shows bright, bead-like highlights around the water
([highlight capture](../../Saved/Playtests/Codex_HighlightReview_20261005_Full/Screenshots/005_Tideglass.png)).
`AIslandPoolRippleEffect` is a plausible source: each automatic rain/wind event arranges eight
moving point lights into a ring. The source had already reduced automatic peaks from 18 to 3.5 for
rain and 14 to 4.5 for strong wind, but the capture does not record its loaded module revision, so
it cannot prove those values were active. As a reversible follow-up, automatic peaks are now
reduced to 0.75 and 1.35. Deliberate resident/player inspection keeps its 55-intensity ripple so an
intentional action remains legible. Regression assertions preserve that distinction. The numeric
change is not a confirmed visual fix: rebuild and capture matched calm/rain views before deciding
whether these lights caused the orbs or whether the ambient cue is now too faint.

### Build follow-up

The bounded UE 5.8.3 scratch build reached its compile actions, so the two inaccessible `dotnet`
process entries were not blocking this attempt. It failed while compiling unrelated current
`IslandArrangement.h` changes owned in the coordination table: MSVC reported C4430 on the two
`GENERATED_BODY()` lines and C4430/C2143 around `UCLASS()` and `AIslandArrangement`. The full log is
`Saved/CompileScratch/Codex_UnderstoryVerify_20261002/Project/Saved/Logs/Codex_TideglassGlintBuild_20261005.log`.
No runtime capture of the softened automatic ripples was produced. Leave the active arrangement
work untouched; once its owner resolves the compile failure, rebuild and capture matched calm/rain
Tideglass views before deciding whether the bright beads are gone or have another source.

A follow-up with UBT's `-NoPCH` option reproduced the same `IslandArrangement.h` reflection-macro
errors. It also forced a 136-action, non-unity rebuild and exposed unrelated missing-include errors
in other translation units; the diagnostic was intentionally interrupted at action 35. This is not
a complete no-PCH build or a source fix. Do not use `-NoPCH` as the validation workaround; resume
with the normal target after the arrangement header issue is resolved.

A later `Build.bat` probe used the isolated scratch project with the editor absent. Its UBT child
remained alive for about two minutes with 0.14 seconds of CPU time, waiting threads, no compiler
children, and no fresh project log. The command was interrupted; this establishes a startup stall,
not its owner or exact mutex. The two inaccessible zero-thread `dotnet` process entries remain.

Direct compilation did succeed once the cached MSVC response files were invoked from UE's
`Engine/Source` working directory: `IslandPoolRippleEffect.cpp`, `IslandWeatherTests.cpp`, and
`IslandEcologyTests.cpp` all compiled into an isolated output folder. Earlier `/Y-` and VS 14.38
attempts failed because they used the wrong working directory or PCH compiler, respectively. A
manual full-module link using the current scratch response then failed with 23 unresolved project
symbols, including ForestStag, TideglassDragonfly, ListeningStonePresentation, and
WindArchPresentation methods. The scratch module, PDB, and replaced object files were restored and
SHA-256 checked against their backups. This diagnostic is superseded by the later successful
working-tree UE 5.8.3 build below.

At 22:07 on 2026-10-05, a normal UE 5.8.3 `Build.bat` invocation targeted the actual working-tree
project. The sandboxed UBT process stalled while accessing its default log area; a direct `-Help`
probe exposed `UnauthorizedAccessException` enumerating `C:\Users\freel\AppData\Local\UnrealBuildTool`.
An approved elevated build then completed all 17 actions, including the real module link, in 68.91 s.
Headless `NightEcology` and `IslandWeather` automations both passed. `IslandLichen` reached its
assertions but failed only `Dusk is part way` at 19:00: the current night curve is already fully on
by then, so the test's dusk sample is mistimed (18:30 is a valid transitional sample). Claude owns
that test and the day/night implementation. See
`Saved/Logs/Codex_TideglassGlint_Automation_20261005.log`,
`Saved/Logs/Codex_IslandWeatherGlint_20261005.log`, and
`Saved/Logs/Codex_IslandLichen_20261005.log`. These tests used `-NullRHI`.

Two bounded real-game captures followed the build with `-CaptiveSkyDisableAgentThinking`, a
one-request cap, and isolated data roots. The clear/unforced run stopped at 120.3 real seconds
with zero model requests; its Tideglass frame is
`Saved/Playtests/Codex_Highlight_Tideglass_20261005/Screenshots/002_TideglassPool.png` (Day 1,
17:06). A separate forced-storm run stopped at 60.0 seconds with zero requests; its Tideglass
frame is `Saved/Playtests/Codex_TideglassStorm_20261005/Screenshots/005_Tideglass.png` (Day 1,
17:24). Both frames still show several bright, floating sphere-like highlights, so the art pass
has not resolved the visible issue. They use different routes/times and do not isolate ripple
intensity; the spheres' source remains unconfirmed. Do not present these as a matched visual fix.

## Transient Tideglass capture and foliage spot check (2026-10-07)

The first new noon capture intentionally omitted `-TideglassMaterial`, so its white pool was the
documented fallback sphere—not a regression. The local `/Game/Materials/M_TideglassPool_Lively`
asset loaded correctly when passed to the transient preview. A calm-collection Tideglass view
with the temporary `meadowunderstory` layer is saved at
`Saved/Viewpoints/2026-10-07_042902_h12.0/02_Tideglass.png`; the surface is teal and the capture
leaves the saved world state untouched. Generic blockout props and sparse distant coverage still
dominate the wide composition, so this is evidence of the material preview, not a polished
highlight.

The Tideglass and Tideglass-ground-detail captures passed the 30 FPS *capture-screen* gate: their
reported p95 frame rates were 52.09 and 50.11 FPS over 50 valid intervals. The ground-detail
camera still looks through foreground plants; switching from 21 `meadowunderstory` HISM components
to four `grass` components changed that p95 from 50.11 to 47.66 FPS and did not make the water legible
from this low framing. These are offscreen SceneCapture measurements, not actual-gameplay or
target-hardware guarantees. No Game/PIE session was run for this check.
