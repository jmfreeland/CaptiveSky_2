# Tideglass fixed-view Game checkpoint (2026-10-08)

A standalone UE 5.8.3 Game sample loaded the Island and held the existing
Tideglass motion-probe composition. Agent thinking and Python were disabled,
the data root was isolated under `Saved/Playtests/Codex_TideglassCurrent_20261008`,
and the run ended itself at its 120-second real-time cap with zero model
requests. Startup required a warmed-cache retry: the first attempt hit its
300-second startup watchdog during first-run engine material/texture shader
compilation and DDC work; the retry reached the Island in 89.7 seconds. No
interactive editor process was used or changed.

The CSV profiler captured 600 frames over 10.24 seconds at 1600x900. Frame time
was 17.17 ms mean, 14.80 ms p50, 24.83 ms p95 (about 40.3 FPS), and 171.78 ms
maximum. This stationary, fixed-view sample clears the 30-FPS p95 floor, but
the maximum exposes a hitch tail. It is one short sample, not proof that PIE,
travel, weather transitions, or target hardware meet the same threshold.

## Hitch-tail drilldown

The same 600 numeric frames in [`Profile(20261008_010737).csv`](../../Saved/Profiling/CSV/Profile%2820261008_010737%29.csv)
contain 19 frames over 33.33 ms and nine over 50 ms. Sixteen frames exceed
33.33 ms on the Render Thread, compared with three on the Game Thread and one
on the GPU. In the 171.78 ms maximum-frame row, GameThreadTime is 20.51 ms,
RenderThreadTime is 139.85 ms, GPUTime is 23.05 ms, and RHIThreadTime is 9.73
ms. The largest exclusive Render Thread samples in that row are EventWait
(50.23 ms), RDG (39.81 ms), and EventWait/Visibility (15.70 ms); the Game
Thread also spends 117.73 ms in EventWait. These overlapping thread samples
are not additive, and a single CSV row does not establish the underlying wait
owner or root cause.

This points toward variable render-thread synchronization/RDG work in this
capture, not a demonstrated GPU-bound frame or a proven ground-cover-density
cause. Do not tune foliage density or culling based on the maximum frame alone.
The longer [controlled Insights record](2026-10-04-actual-gameplay-profile.md)
already compares Lumen, CPU ground-cover sway, occlusion queries, and the RHI
thread. It linked some earlier ~0.45–0.48 second stalls to CPU instance updates
and task/sync-point waits, but this shorter 10/8 hitch is not proven to share
those causes. Avoid repeating those broad ablations. If the hitch recurs after
the current foliage changes settle, capture an event-level trace of that exact
default config with screenshots disabled and correlate Render Thread EventWait
/ RDG against `GameThreadWaitForTask`; separately keep the bounded moving-PIE
check distinct from this fixed-view profile. This does not change the p95
result or the separate need to improve the broad view's ecological composition.

The [Game screenshot](../../Playtests/Codex_TideglassCurrent_20261008/Screenshots/003_Tideglass.png)
shows a reflective pool, a dense and partly tangled foreground, broad sparse
brown middle distance, and thin distant tree silhouettes. The camera makes the
landscape's structural issue clear: coverage is not yet organized into an
intentional wet edge, open circulation, meadow masses, and woodland transition.
This is a Game render, unlike an editor-world preview, but it is not a species
or materials beauty pass.

## Next checkpoint

Keep the matched 1600x900 camera and 600-frame capture as a comparison while
the ecological-band work evolves. Retain separate results for the static broad
view and ordinary moving/PIE play; do not infer gameplay performance from this
fixed camera. Art-direct one narrow wet-edge band and open circulation lane,
then recheck both the composition and the p95 target before generalizing zones.

Log: [`SpectatorStartup_20261008_010524_634.log`](../../Saved/Logs/SpectatorStartup_20261008_010524_634.log).

## Solo-layer composition captures

The updated `CaptiveSky2.Visual.Viewpoints` harness was exercised in a no-world-
state editor-world capture at 11:00 with transient Tideglass dragonflies and the
temporary `M_TideglassPool_Lively` water preview. It passed and reported
1,780,640 nonblocking ground-cover instances. The `meadow` layer capture
measured 49.09 FPS p95 SceneCapture throughput; the separate `woodland` layer
capture measured 52.04 FPS p95. These offscreen layer diagnostics are not Game
or PIE performance measurements.

The [meadow-only frame](../../Saved/Viewpoints/2026-10-08_013038_h11.0/02_Tideglass.png)
shows continuous, tangled ground cover encroaching on the pool with no clear
circulation lane. The [woodland frame](../../Saved/Viewpoints/2026-10-08_013218_h11.0/02_Tideglass.png)
shows a sparse canopy over a broad bare brown midground. Together, these support
the existing diagnosis: the next composition should be a deliberately
intermediate habitat—an open lane and narrow wet edge in the foreground, grouped
meadow masses through the middle, and a denser but readable woodland edge—rather
than globally adding more instances. The temporary water material reads cyan and
the Wind Arch proxies remain blockout shapes; these captures do not validate the
current Game water material or final landmark art.

A third no-world-state capture isolated only the `groundplants` layer, using the
now-validated `-TideglassDragonflies` wrapper option. The viewpoint automation
passed at 47.24 FPS p95 SceneCapture throughput; this remains an offscreen
diagnostic, not Game or PIE performance evidence. The [ground-plants-only
frame](../../Saved/Viewpoints/2026-10-08_014251_h11.0/02_Tideglass.png) still
reads as nearly continuous foreground cover, with no clear circulation lane.
That shows the issue is spatial organization, not merely species choice: the
next production pass needs to reserve an open corridor and confine wet-edge
plants to a narrow band rather than scattering them uniformly across the view.
The transient pool material remains a preview only.

The initial wrapper attempt failed its dragonfly close-up because the script did
not forward the harness's daytime-dragonfly preview flag. `Capture-Viewpoints.ps1`
now exposes `-TideglassDragonflies`, validates daylight and its compatibility
with the night-firefly preview, and the full meadow-layer capture passed through
the wrapper at 46.71 FPS p95 SceneCapture throughput. An hour-20 negative check
was rejected before launch. Log: [`Codex_TideglassMeadowWrapper_20261008.log`](../../Saved/Logs/Codex_TideglassMeadowWrapper_20261008.log).
No saved map or world state was changed and no model request was made.

## Shared wetland layer checkpoint

The current working-tree foliage preview was exercised without changing its
source or map. The UE 5.8.3 target was already up to date; headless
`CaptiveSky2.Agent.IslandWeather` passed, and the no-world-state
`CaptiveSky2.Visual.Viewpoints` meadow-layer capture passed with transient
dragonflies. It reported 7 Typha plus 3 Phalaris, 559.6 cm minimum pool-edge
clearance, and 440.8 cm minimum shared plant spacing. The `meadow` diagnostic
kept 19 HISM components visible. SceneCapture p95 throughput was 50.17 FPS at
the overview, 47.51 FPS at ground detail, and 51.16 FPS for the close
dragonfly frame. These are offscreen editor captures only; they do not verify
Game/PIE performance or the authored landscape/water materials.

The [current meadow frame](../../Saved/Viewpoints/2026-10-08_035323_h11.0/02_Tideglass.png)
still reads as dense foreground cover over a broad sparse brown midground; the
white pool and blocky landmark shapes are editor-world placeholders, not final
Game art. The layer isolation makes the next art direction clearer: reserve a
legible circulation opening and group a visibly distinct wet-edge band, rather
than increasing total meadow coverage. Logs: [IslandWeather regression](../../Saved/Logs/Codex_SharedIslandWeatherValidation_20261008.log)
and [Viewpoints capture](../../Saved/Logs/Codex_SharedWetEdge_Meadow_Dragonflies_20261008.log).

## Live Simulate observation

After the local Unreal MCP bridge was restored, the actual Island ran in
Simulate-In-Editor for about 83 seconds. The editor launch used a separate
world-data root, disabled resident thinking and set the model-request cap to
zero; the session was stopped manually and confirmed idle afterward. A live
viewport capture at the Tideglass composition showed runtime ground cover,
the stag, and the raven rather than the offscreen harness's transient proxy
preview. The ground cover still crowds the foreground and does not form a
clear circulation lane; the pool surface reads pale and flat in this Simulate
view, so verify it again in Game before judging the authored runtime material.

The editor log reports a 306 cm pool-edge clearance and “10 nonblocking
cattails,” while the passing wetland composition automation reports 7 Typha
plus 3 Phalaris. Treat the count as consistent but the runtime species label
as stale instrumentation; the shared `IslandWeather` work was not edited.
This was a brief visual observation, not an FPS/profile measurement or a
30-FPS gameplay claim. Log: [bounded MCP editor session](../../Saved/Logs/Codex_EditorMcpRecovery_20261008.log).

## Current-source bounded Game profile (2026-10-08)

A scratch UE 5.8.3 Game launch loaded the exact current `IslandWeather.h`
working-tree contents (SHA-256 matched the scratch copy) at 11:00 with the
Listening-Stones close camera held stationary. Agent thinking and Python were
disabled, the world-data root was isolated, and the play session ended itself
at 75.2 real seconds with zero model requests. The open editor and unsaved
level were left alone. Cold engine/shader initialization took 320.75 seconds;
the Game log also recorded 300 PSO-creation hitches with none precached. Logs:
[`CurrentWetEdgeProfile.log`](../../Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/Project/Saved/Logs/CurrentWetEdgeProfile.log)
and CSV [`Profile(20261008_104857).csv`](../../Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/Project/Saved/Profiling/CSV/Profile%2820261008_104857%29.csv).

Runtime scatter reported 1,779,877 ground-cover instances under the 1.92M
budget, 359 Fab meadow flowers across eight species, 14,727 trees, 15,063
shrubs, and 1,487 woodland rhododendrons. This confirms the existing local
pack already contributes substantial species variety; it does not confirm
the user's separate Fab Megaplants library is installed in this project.

The requested 600-frame CSV window was cut short by the 75-second gameplay
cap and contains 406 numeric frame rows, so this is an exploratory sample, not
a completed performance gate. In those rows FrameTime p50 was 17.45 ms, p95
33.63 ms (29.7 FPS), and max 1,693.71 ms. After discarding the first 120
captured rows, the remaining 286 had p95 26.45 ms (37.8 FPS) and max 78.68
ms. The first-window tail coincided with cold PSO creation; it must not be
presented as a stable traversal result. The game screenshot
[`001_Listening_Stones_Close.png`](../../Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/Project/Saved/Playtests/CurrentWetEdgeProfile/Screenshots/001_Listening_Stones_Close.png)
is a close landmark view, not the broad Tideglass composition or moving PIE.

The complete 600-frame follow-up is recorded below. Next, capture the 11:00
broad Tideglass view and a distinct moving-PIE traversal before deciding
whether to tune density or composition. No foliage source, imported asset, or
world-state file was changed in either diagnostic.

## Complete 600-frame follow-up (2026-10-08)

The same scratch UE 5.8.3 Game setup and 11:00 Listening-Stones camera completed
all 600 requested CSV frames in 14.49 seconds after the 10-second spectator
delay. Agent thinking and Python were disabled, the command requested zero
model requests, and the world data remained isolated. The safety subsystem
logged its deliberate one-request minimum ceiling; no provider-call entry
appears in this run's log, though the process was stopped after the CSV was
finalized and so has no normal-shutdown request tally. It was stopped after the
capture (about 80 seconds of its 180-second real-time gameplay cap). The user's
editor was not touched. Startup again took 324.74 seconds because engine
material shaders were absent from the read-only shared DDC; 500 PSO creation
hitches were reported, none precached. The project's current configuration
also reports `r.D3D12.PSO.DiskCache=0`.

Across the complete 600-frame close-camera sample, FrameTime mean was 24.28 ms,
p50 17.41 ms, p95 33.20 ms (30.1 FPS), and max 970.46 ms. Thirty frames were
over 33.33 ms and fourteen over 50 ms. The full window therefore clears the
30-FPS p95 floor only narrowly and retains a severe hitch tail. After the first
120 captured frames, the remaining 480 measured 17.20 ms p50, 29.06 ms p95
(34.4 FPS), 56.76 ms max, with sixteen frames over 33.33 ms and four over 50
ms. The warm-window result clears the target more comfortably, but it remains
a stationary close view; it is not moving PIE or a broad-composition check.

Log: [`CurrentWetEdgeProfile600.log`](../../Saved/Logs/CurrentWetEdgeProfile600.log).
CSV: [`Profile(20261008_110102).csv`](../../Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/Project/Saved/Profiling/CSV/Profile%2820261008_110102%29.csv).
Diagnostic frame: [`001_Listening_Stones_Close.png`](../../Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/Project/Saved/Playtests/CurrentWetEdgeProfile600/Screenshots/001_Listening_Stones_Close.png).

Next: reproduce the user's ordinary moving route under the same current source,
keeping cold PSO and warm gameplay windows separate. Then compare the same
11:00 broad Tideglass view and its composition against the foliage WIP before
changing scatter density. Any persistent PSO-cache fix should be a separately
measured project-config experiment, not bundled into this environment sample.

## PSO-cache config probe (2026-10-08)

A disposable scratch-project config added
`[ConsoleVariables] r.D3D12.PSO.DiskCache=1`; the repository's normal config and
the open editor were untouched. This did **not** enable the D3D12 cache: the
startup log says the CVar was deferred as a dummy variable, then D3D12 RHI
explicitly reports `Not using pipeline state disk cache per
r.D3D12.PSO.DiskCache=0`. During world startup it still counted 500 PSO
creation hitches, zero precached. Treat this as a failed toggle, not a cache-on
benchmark. A second isolated launch also passed
`-r.D3D12.PSO.DiskCache=1`; the command line was recorded and the startup log
again created a deferred value of 1, but D3D12 RHI still reported the cache
disabled at 0. That run was stopped before world startup because it could not
test persistence. Do not repeat the same config/command-line toggle; first
investigate whether this installed UE 5.8.3 build hard-disables this path or
whether another supported PSO precaching mechanism is intended.

The isolated profile completed all 600 CSV frames in 14.61 seconds after its
10-second spectator delay. In the full stationary close-camera window,
FrameTime mean was 24.49 ms, p50 17.45 ms, p95 32.26 ms (31.0 FPS), and max
1,530.77 ms; 27 frames exceeded 33.33 ms, 15 exceeded 50 ms, and 8 exceeded
100 ms. After the first 120 frames, the remaining 480 had mean 18.75 ms, p50
17.18 ms, p95 30.33 ms (33.0 FPS), max 67.05 ms, with 12 frames above 33.33
ms and 6 above 50 ms. The warm-window p95 clears the 30-FPS floor, but its
remaining hitch tail and the full-window spike mean this is not a gameplay
pass or moving-PIE result.

This run used a writable local DDC, but first-use project shader/material
processing still took about 7 minutes 17 seconds before the Island became
ready; at peak the game process used about 7.8 GB and 16 shader workers were
active. The run used the same 11:00 Listening-Stones view, disabled resident
thinking and Python, requested zero model calls, and wrote only to scratch
world data. It was stopped after CSV finalization. Log:
[`CurrentWetEdgePSODiskCache.log`](../../Saved/Logs/CurrentWetEdgePSODiskCache.log).
CSV:
[`Profile(20261008_111647).csv`](../../Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/Project/Saved/Profiling/CSV/Profile%2820261008_111647%29.csv).
Frame:
[`001_Listening_Stones_Close.png`](../../Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/Project/Saved/Playtests/CurrentWetEdgePSODiskCache/Screenshots/001_Listening_Stones_Close.png).

Next: profile the ordinary moving-PIE route and broad Tideglass composition
with the warmed DDC, keeping first-use PSO hitches separate from steady travel.
Investigate UE's supported precache mechanism only after capturing that route;
the two cache-enable paths above both failed to change RHI state.

## Grounded Aster movement smoke test (2026-10-08)

An isolated UE 5.8.3 NullRHI Game run invoked
`Island.MoveProbe Agent_Aster_01 Wander` with resident thinking and Python
disabled, an isolated world-data root, a 120-second real-time cap, and zero
model requests. Aster began falling at Z=3695 cm; the probe waited until the
pawn landed at Z=2823 cm before asking the normal controller to wander. The
controller selected a nav route, rejected 11 capsule-blocked candidates, then
reached its selected destination after 1,793 cm of travel in 3.5 simulated
seconds. The bounded session ended normally after 28.5 real seconds with zero
model requests.

This confirms one grounded autonomous move under current working-tree source;
it does not test a landmark interaction, player input, rendered appearance,
or gameplay frame rate. A separate immediate `MoveTo ListeningStones` probe
failed while Aster was still falling, so that result is not evidence that the
landmark route itself is unreachable. The same successful log emitted the
UBT AutoSDK return code discussed in
[`the .NET exception note`](2026-10-08-ue-ubt-dotnet-exception.md), and the
known RecastNavMesh/CrowdFollowing teardown warning appeared after the move
completed.

Evidence: [`CurrentAsterGroundedWander.log`](../../Saved/Logs/CurrentAsterGroundedWander.log).
Next: make any future direct ground-movement probe wait for landing first,
then test one tagged landmark arrival and interaction in a render-enabled
session. Keep that separate from the still-unverified user-controlled PIE
route and 30-FPS target.

## Rendered Aster-to-Listening-Stones interaction (2026-10-09)

An isolated UE 5.8.3 D3D12 Game run released the startup `AsyncLoadLock`, built
dynamic navigation, then issued Aster's normal `MoveTo` followed by the normal
`Interact` action for `ListeningStones`. Agent thinking and Python were
disabled; the world-data directory was isolated; the 120-second real-time cap
ended normally after 102.0 seconds with zero model requests. The probe waited
for the corrected nav rebuild before dispatch and did not use the open editor.

Aster moved 1,377 cm in 3.0 simulated seconds from his normal runtime start to
the stone marker. The controller reported a complete 15 m route (six points),
then the normal interaction returned: “Your inspection woke a quiet, layered
resonance in the ListeningStones,” tuned to local wind and fading after a few
seconds. The isolated Aster memory file recorded that action result. The
controller's status string immediately appended its 300-second inspection
cooldown after the successful result; that cooldown message is not a failed
interaction. The interaction explicitly leaves no lasting world change. The
first probe used `-nosound`, so it did not validate audio output. A subsequent
isolated close-camera run with normal audio enabled captured the stones before
and about 1.3 seconds after the action; the latter frame shows a brief cyan
highlight on the outer stone forms. This verifies that the transient light
response renders, though it is subtle and the underlying stone silhouettes
remain obvious blockout-style prototypes. It does not confirm what a human
listener hears on their output device.

The same rendered session captured all 600 CSV frames at 1600x900 over 10.37
seconds, including the short walk and interaction. FrameTime was 17.27 ms mean,
16.45 ms p50, 21.05 ms p95 (47.5 FPS), and 93.01 ms maximum. This clears the
30-FPS p95 screen in this short fixed-view sample, but its hitch maximum and
stationary spectator camera mean it is not a moving-PIE or user-controlled
gameplay pass.

Log: [`AsterListeningStonesInteractD3D12Final.log`](../../Saved/NavBoundsTest/Project/Saved/Logs/AsterListeningStonesInteractD3D12Final.log).
CSV: [`Profile(20261009_080330).csv`](../../Saved/NavBoundsTest/Project/Saved/Profiling/CSV/Profile%2820261009_080330%29.csv).
Isolated observation: [`memory.jsonl`](../../Saved/NavBoundsTest/AsterListeningStonesInteractionWorldRetry/Agents/Agent_Aster_01/memory.jsonl).
Pre-interaction: [`002_Listening_Stones__Resonance_Close.png`](../../Saved/NavBoundsTest/Project/Saved/Screenshots/ListeningStonesGlowPeak3s/002_Listening_Stones__Resonance_Close.png).
Resonance frame: [`003_Listening_Stones__Resonance_Close.png`](../../Saved/NavBoundsTest/Project/Saved/Screenshots/ListeningStonesGlowPeak3s/003_Listening_Stones__Resonance_Close.png).

## Transient cairn silhouette refinement (2026-10-09)

The first interaction close-up showed that equal-sized stacked rocks still
read as soft blockout towers. The transient presentation now tapers each stack
from full footprint at the base to 70% at the crown (previously 90%) and gives
the layer centers a little more lateral stagger. The map proxies, collision,
navigation, and shared persistent world remain unchanged.

The root UE 5.8.3 editor target compiled and linked successfully. Focused
`CaptiveSky2.Agent.ListeningStonePresentation` automation passed, including a
new check that each cairn narrows toward its upper stones, alongside the
existing proxy-bounds, collision, navigation, and transient-lifetime checks.
A 1600x900 afternoon Game capture of the current root project also completed;
it used an isolated data root, disabled resident thinking and Python, and ended
at its 60-second cap with zero model requests. The upper taper is visible in
the render, but the stones still look like oversized rounded prototypes rather
than authored cairns. No content asset or map was edited.

Test log: [`ListeningStonePresentationAutomationRetry.log`](../../Saved/NavBoundsTest/Project/Saved/Logs/ListeningStonePresentationAutomationRetry.log).
Game log: [`ListeningStoneCairnShapeGame.log`](../../Saved/NavBoundsTest/Project/Saved/Logs/ListeningStoneCairnShapeGame.log).
Render: [`005_Listening_Stones__Resonance_Close.png`](../../Saved/NavBoundsTest/Project/Saved/Screenshots/ListeningStoneCairnShape/005_Listening_Stones__Resonance_Close.png).

Next: keep the small Tripo cairn distinct from the tall Listening Stones. If
those totems are to receive authored silhouettes, inspect a slender standing
stone or a set of individual stackable rocks before changing their transient
presentation. Once the shared landscape material work lands, repeat the
Tideglass composition captures and separately profile a moving-PIE route.

## Tripo StoneCairn candidate audit (2026-10-09)

The imported source preview and local import report confirm a well-textured,
mossy pile of rounded stones. Its LOD0 has 6,361 vertices, a single material
slot, box collision, and bounds of 103 × 110 × 85.3 cm. The in-world close
capture shows this asset already placed as the separate `StonesCairn` prop;
it reads as a small grounded cairn, partly screened by the current foreground.

It is not a like-for-like replacement for the three transient Listening Stones:
their saved proxy heights are 224, 268, and 302 cm, with narrow footprints.
Uniformly scaling this complete pile to those heights would make it roughly
2.7–3.9 m across, while fitting its width would leave it under a metre tall.
Keep the Tripo asset as the small mossy cairn rather than stretching it or
duplicating its already-stacked geometry across those totems. The existing
Starter Content rock stack remains the reversible runtime treatment for the
tall silhouettes.

If those tall landmarks are to become authored meshes, the useful next asset
shape is either a slender standing stone at roughly the proxy dimensions, or
several separate low-profile rocks suitable for the current layered stack.
No `Content/` package or map was changed during this audit.

Source preview: [`preview.png`](../../Saved/CaptiveSky/Tripo/StoneCairn/preview.png).
Import report: [`import.json`](../../Saved/CaptiveSky/Tripo/StoneCairn/import.json).
Current in-world comparison: [`005_Listening_Stones__Resonance_Close.png`](../../Saved/NavBoundsTest/Project/Saved/Screenshots/ListeningStoneCairnShape/005_Listening_Stones__Resonance_Close.png).

## Current Tideglass foliage and dragonfly view (2026-10-09)

The current shared source passed `CaptiveSky2.Visual.Viewpoints` in an isolated
UE 5.8.3 editor process. The run used `-ViewpointOnly=Tideglass
-ViewpointGroundCover -ViewpointTideglassDragonflies -ViewpointNoWorldState
-ViewpointHour=11`, disabled agent thinking, and set the model-request ceiling
to zero. The 30-FPS interactive-frame wait eventually sustained 30.39 FPS for
five seconds after 29 seconds of waiting. The capture gate reported:

| View | Wall-clock capture FPS | p95 frame throughput | Valid intervals |
|---|---:|---:|---:|
| Tideglass overview | 53.23 | 34.09 FPS | 50 |
| Ground detail | 59.62 | 58.15 FPS | 50 |
| Dragonfly close-up | 59.32 | 57.36 FPS | 50 |

The preview contained exactly three transient daytime dragonflies and the
current wet edge contained seven Typha cattails plus three Phalaris grass
instances. The `Viewpoints` automation completed successfully, and preview
actors were destroyed. These offscreen SceneCapture results clear the current
30-FPS p95 screen, but do not establish moving gameplay performance.

Visual inspection found that the overview still has a large white pool proxy,
gray vertical blockout landmarks, and a broad exposed brown ground surface.
The close dragonfly is easier to inspect but still reads as a procedural
placeholder: smooth body ellipsoids and pale, leaf-like wing membranes. Neither
image is a finished highlight. Landscape materials and variety are already
being worked in the shared tree; keep this capture as a before image and
revisit the composition after that work lands. Do not scale the fauna population
to compensate for the silhouette.

The first launch attempt aborted before the Island loaded because the shader
compiler could not write beneath the default user-profile working directory.
The successful retry pointed `-ShaderWorkingDir` into the ignored scratch
directory. It used no persistent world-state and ran the editor in isolation;
the user's main editor was not touched.

Log: [`Codex_CurrentTideglassGate_20261009b.log`](../../Saved/CompileScratch/Codex_CurrentTideglassGate_20261009/Codex_CurrentTideglassGate_20261009b.log).
CSV: [`Profile(20261009_073324).csv`](../../Saved/CompileScratch/Codex_RippleTint_20261009/Saved/Profiling/CSV/Profile%2820261009_073324%29.csv).
Overview: [`02_Tideglass.png`](../../Saved/CompileScratch/Codex_RippleTint_20261009/Saved/Viewpoints/2026-10-09_073406_h11.0/02_Tideglass.png).
Ground detail: [`02a_TideglassGroundDetail.png`](../../Saved/CompileScratch/Codex_RippleTint_20261009/Saved/Viewpoints/2026-10-09_073406_h11.0/02a_TideglassGroundDetail.png).
Dragonfly: [`02e_TideglassDragonflyClose.png`](../../Saved/CompileScratch/Codex_RippleTint_20261009/Saved/Viewpoints/2026-10-09_073406_h11.0/02e_TideglassDragonflyClose.png).

Next: once the landscape-material WIP settles, rerun the same views against its
updated composition. Separately profile a real moving resident/player route
with warmed shaders; the offscreen capture gate is not that test.

## Bounded opening spectator-view profile (2026-10-09)

An isolated UE 5.8.3 D3D12 Game run loaded the current Island and captured 120
CSV frames during the opening `Shore Approach` spectator view at 1280×720. This
is a slow establishing-camera drift, not a player/resident traversal. Agent
thinking was disabled, the data root was isolated, and the configured limits
were 45 real seconds and zero model requests. The session recorded zero
requests and closed normally after 45.2 seconds from safety-subsystem
initialization.

The 120-frame sample measured 14.77 ms average frame time, 13.82 ms median,
19.32 ms p95, 30.56 ms p99, and 46.43 ms maximum (67.7 average / 51.7 p95 FPS).
That clears the project's 30-FPS p95 floor in this short spectator segment.
It is not a moving traversal, a broad gameplay sample, or a packaged build
result. Unreal still reported 400 unprecached D3D12 PSO creation hitches
during startup, so treat the peak frame and first-use hitching separately from
the measured post-startup sample. The earlier cold screenshot still displayed
“Preparing Shaders” and is not a highlight or a visual baseline.

The game log also recorded `UBT AutoSDK ReturnCode: -532462766`; it nevertheless
loaded the Island, accepted the CSV profile command, and exited normally. This
adds another successful runtime correlation, not a root-cause diagnosis for the
separate `dotnet.exe` dialog. The command-line request limit of zero is clamped
to one by the current subsystem; disabled agent thinking and the observed zero
request count kept this run model-free in practice.

Log: [`Codex_MoveWarm_20261009.log`](../../Saved/CompileScratch/Codex_VegetationLayerAudit_20261009/Saved/Playtests/Codex_MoveWarm_20261009/Codex_MoveWarm_20261009.log).
CSV: [`Profile(20261009_131932).csv`](../../Saved/CompileScratch/Codex_VegetationLayerAudit_20261009/Saved/Profiling/CSV/Profile%2820261009_131932%29.csv).

Next: profile a player-controlled route or a resident moving through the dense
vegetation with warmed caches; keep the same strict time/request safeguards.
