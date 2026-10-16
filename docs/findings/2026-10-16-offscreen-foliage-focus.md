# View-local resident foliage sway focus — 2026-10-16

`AIslandWeather::UpdateGroundCoverSway` previously added every `ACharacter` in
the world to its resident-ground focus set, even when a resident had never been
visible to the active view. That created extra CPU transform work and
plant-bending neighborhoods which could not contribute to the current image.
The focus set now keeps player-controlled characters regardless of mesh
visibility and includes other characters only while their skeletal mesh was
recently rendered. Camera and active transient-gust focus points are unchanged;
nearby visible residents still bend the plants around their ground reference.
When a resident leaves view, the normal previous-index restore path returns
that resident's formerly swayed plants to their immutable base transforms.

The policy is isolated in `ShouldIncludeResidentInSwayFocus` and covered by the
`CaptiveSky2.Agent.IslandWeather` regression source: the controlled character
is retained, recently rendered wildlife is retained, and an offscreen
non-player is excluded. The opt-in console variable
`CaptiveSky.Island.FoliageSwayFocusTelemetry` (default `0`) reports candidate,
included, and excluded character counts alongside transform totals once per
sway pass.

The UE 5.8.3 editor target compiled and linked successfully in the existing
isolated scratch project (34 actions, including `IslandWeather.cpp` and
`IslandWeatherTests.cpp`). The automation test was not run, and no Unreal
Editor/Game process was started: the matching editor symbols remain absent,
and project-opening startup is still gated on symbolizing the retained Core
crash dump. No performance gain is claimed yet. After that gate is cleared,
repeat the same bounded 1600x900 Tideglass Game profile with the telemetry
enabled; compare included resident/transform counts and p50/p95 frame, Game,
Render, and RHI timings against the recorded full-focus and sway-disabled
baselines. Keep the local plant response and full species composition in the
visual acceptance review, along with the 30-FPS floor.

## Focused automation follow-up (2026-10-16)

The current source and test were copied into the isolated
`Saved/CompileScratch/Codex_RavenCrabAttention_20261008` project and verified
identical to the main-tree files before running. The UE 5.8.3 scratch editor
target was up to date, and `CaptiveSky2.Agent.IslandWeather` completed under
NullRHI with exit code 0, thinking disabled, zero model requests, and a
60-second realtime cap. The focused assertions confirm that a player-controlled
character is retained even with a hidden mesh, a recently rendered resident is
retained, and an offscreen non-player does not create a sway neighborhood.
The teardown emitted `DestroyActor: World has no context` warnings for several
fixture actors, but the automation result was `Success`; these warnings did not
fail the test. This verifies the view-selection policy, not a sustained FPS
gain or its visual quality. The matched real-Game performance comparison and
visual check remain outstanding.

## Bounded real-Game foliage-focus comparison (2026-10-16)

Two UE 5.8.3 real-RHI standalone Game runs used the isolated scratch project
(scratch binaries and save data, shared project Content), `/Game/Maps/Island`,
the five-entry Tideglass motion probe (same camera repeated), 1600x900, noon,
and a 400-frame CSV beginning 15 seconds after spectator startup. Agent
thinking was disabled, the model-request cap was zero, and each play session
ended at its 90-second realtime cap with exit code 0. The only intended
comparison variable was `CaptiveSky.Island.FoliageSwayFocusRadiusCm`, explicitly
confirmed in each log: 100 cm versus 3000 cm.

| Sway focus | Frame time p50 / p95 / max | Frames >33.3 ms | Game Thread p50 / p95 | Render Thread p50 / p95 | GPU p50 / p95 | Max instance-transform updates |
|---|---:|---:|---:|---:|---:|---:|
| 100 cm (current default) | 15.42 / 25.23 / 53.67 ms | 12 / 400 | 6.57 / 7.88 ms | 15.39 / 19.77 ms | 12.62 / 17.64 ms | 456 |
| 3000 cm (wide-focus control) | 16.06 / 109.14 / 162.60 ms | 42 / 400 | 6.16 / 25.09 ms | 15.85 / 84.47 ms | 12.94 / 20.29 ms | 28,607 |

Both runs retained the same scene population (median 1,812,628 dynamic and
678,932 static instances), so the 100-cm result did not come from reducing
vegetation density or removing species. It stayed under the 33.3-ms p95
30-FPS budget in this sample while sharply limiting transform-update bursts.
The paired screenshots show the same overall composition; a still image cannot
prove that short-range foliage motion is perceptually equivalent.

Treat this as promising but preliminary: the run order was not randomized, the
3000-cm condition had unusually large render-thread outliers, and this is one
camera, one short capture per condition on one machine. The medians were close,
so this does not establish a general performance gain or prove the 30-FPS goal
across gameplay. Repeat the pair in alternating order, then profile a player
traversal and inspect close foliage motion before changing the default again.
CSV and logs:
`Saved/CompileScratch/Codex_RavenCrabAttention_20261008/Saved/Profiling/CSV/Profile(20261016_195838).csv`,
`Profile(20261016_200222).csv`,
`Saved/CompileScratch/Codex_RavenCrabAttention_20261008/Saved/Logs/Codex_FoliageFocus_Radius100_20261016.log`,
and `Codex_FoliageFocus_Radius3000_20261016.log`. Paired stills:
`Saved/CompileScratch/Codex_RavenCrabAttention_20261008/Saved/F100/Shots/001_Tideglass.png`
and `Saved/CompileScratch/Codex_RavenCrabAttention_20261008/Saved/F3000/Shots/001_Tideglass.png`.
