# Actual Game-mode performance profile (2026-10-04)

## Why this run

The earlier 46–48 FPS figures came from editor SceneCapture tests and do not
represent sustained gameplay. Claude's PIE observations separately report
roughly 8–13 FPS. This run profiles the actual `-game` spectator path and is the
current direct runtime evidence; it is still an isolated scratch-project run,
not a packaged build or a controlled replay of the user's exact session.

## Capture conditions

- UE 5.8.3 Development editor binary, standalone Game mode, Island map.
- 1600x900 windowed view, Tideglass camera at midday; local Content junctioned
  to the project, with scratch binaries, DDC, saved state, and screenshots.
- Agent thinking disabled to isolate rendering/simulation from network latency;
  play safety remained enabled at 120 real seconds / one maximum model request.
  The run ended normally at 120.6 seconds with zero requests.
- CSV capture stopped automatically after 200 frames (57.22 seconds measured).
  The process then continued only until its real-time safety cap.

CSV: `Saved/CompileScratch/Codex_UnderstoryVerify_20261002/Project/Saved/Profiling/CSV/Profile(20261004_210203).csv`

Log: `Saved/CompileScratch/Codex_UnderstoryVerify_20261002/Project/Saved/Playtests/Codex_ActualGameCSV_20261004/ActualGameCSV.log`

The CSV identifies the host as an Intel i9-13900HX and RTX 4080 Laptop GPU.
Frame-rate instrumentation has overhead, and the result should not be treated
as a hardware-independent benchmark.

## Findings

Across the 200 measured frames:

| Measure | Median | p95 | Max |
|---|---:|---:|---:|
| Frame time | 262.72 ms | 404.68 ms | 1177.22 ms |
| Game Thread | 264.41 ms | 344.31 ms | 1641.23 ms |
| Render Thread | 213.71 ms | 234.86 ms | 2565.77 ms |
| GPU | 23.62 ms | 27.38 ms | 78.33 ms |
| `TickActors` exclusive | 137.33 ms | 184.98 ms | 211.34 ms |
| `STAT_SkinningSceneExtension` exclusive | 146.93 ms | 162.59 ms | 171.81 ms |
| `PrepareDistanceFieldScene` exclusive | 0.10 ms | 0.13 ms | 28.95 ms |
| Instance transform updates per frame | 28,190 | 28,394 | 29,944 |
| GPU-scene instances | 2,313,508 | 2,548,664 | 2,548,664 |

The in-game `stat unit` screenshot from a separate bounded run agrees that this
is CPU-side: it showed 261 ms frame time, 260 ms Game Thread, 214 ms Draw, and
28 ms GPU. The render-thread CSV's distance-field preparation was negligible
in this capture, so the earlier distance-field spikes are not a sufficient
explanation of the current gameplay result. `TickActors`, the skinning scene
extension, and the very high GPU-scene instance count are the leading scopes to
investigate; the counters alone do not identify a single cause.

The world created about 1.78 million ground-cover instances and 14.7k trees,
15.1k shrubs, and 1.5k rhododendrons. `IslandWeather.cpp` is the only project
source containing per-instance transform updates for those foliage groups; its
localized wind/resident sway and spruce sway are therefore a strong candidate
for the 28k update count. A paired sway ablation is still required before
attributing the Game Thread or skinning time to that work. A 6 GB texture-pool
override did not improve a separate quick sample; it is not a controlled
comparison and no global pool setting was changed.

## Next experiment

Build a temporary, default-preserving `-IslandDisableGroundCoverSway` diagnostic
into the isolated target, then repeat the exact 200-frame Game-mode capture.
Compare `FrameTime`, Game/Render Thread times, `STAT_SkinningSceneExtension`,
and `NumInstanceTransformUpdates`. If disabling sway materially helps, replace
the all-resident CPU transform work with a view-local or GPU-driven wind path
while retaining nearby plant response; if it does not, profile the two dominant
thread scopes independently before changing density. Keep a 30 FPS minimum and
verify the full, visually rich vegetation composition before accepting any
tradeoff.

The diagnostic code change was not retained because it could not be built. Both
a normal `Build.bat ... -WaitMutex` probe and an isolated-scratch
`Build.bat ... -NoMutex` probe remained blocked before UBT created its log; only
Codex's own waiting shells were stopped. No editor/game process was running.

UE 5.8 source inspection explains why `-NoMutex` is not a safe bypass here:
`UnrealBuildTool.cs` skips the main single-instance mutex, but
`XmlConfig.ReadConfigFiles` acquires a separate mutex keyed to the shared engine
root before that point. Two existing `dotnet.exe` processes (PIDs 40100 and
52284; created 2026-09-29 and 2026-10-01) remain, but even elevated read-only
process queries cannot reveal their executable paths, command lines, or owners.
This makes the XML-config mutex the likely blocker, not a confirmed owner or
process diagnosis. They were not terminated. The scratch-only diagnostic source
was reverted, so no uncompiled change remains. Resume the matched sway A/B when
the engine-root XML-config mutex is available; no performance fix, compile, or
A/B pass is claimed yet.

## PIE foliage/RHI follow-up (Claude findings, 2026-10-04)

Claude's current PIE notes report about 79 ms/frame at 1080p (roughly 12.7 FPS),
while the editor viewport itself runs near 60 FPS. Hiding ground cover recovers
about 50–60 ms; trees alone are comparatively inexpensive. Resolution scale,
hardware ray tracing, Virtual Shadow Maps, Lumen, water, and cull-distance
changes did not materially improve that PIE result. This points to CPU/RHI work
associated with the large foliage instance/component population, but is not a
species-level attribution.

An experimental Nanite-foliage configuration (`r.Nanite.Foliage=True` with
Voxelize shape preservation on eleven foliage meshes) measured about 84 ms/frame,
so it did not improve on the roughly 79 ms baseline. `ground_06_01` and
`ground_12_01` are the only ground-cover meshes noted as not yet Nanite-enabled;
their status is a candidate for a controlled test, not evidence that enabling
Nanite will help. Do not generalize from mesh capability or instance counts.

These PIE observations are separate from the 1600x900 standalone `-game`
spectator CSV above, which measured 262.72 ms median frame time and implicated
both `TickActors` and the render-thread skinning extension. The test mode,
resolution, instrumentation, and captured workload are not matched, and the
thread-level descriptions differ; treat them as two unresolved runtime profiles,
not as a single reconciled diagnosis. The PIE visibility ablation is the clearest
current evidence that ground cover dominates its own test, while its detailed
RHI/render-thread mechanism remains unverified.

When a safe UE 5.8.3 test session is available, first repeat a warmed PIE baseline
and ground-cover-hidden pair at the same camera, resolution, duration, and
scalability settings. Capture Game, Render, RHI, and GPU timings plus component
and instance counts, and repeat each condition to expose variance. Then test
one structural change at a time (for example, consolidating compatible HISM
components or a reversible density control) against the 30 FPS floor and the
full habitat composition. Avoid another global cull-distance sweep or Nanite
foliage trial without a specific, measurable hypothesis. No source change,
build, or runtime validation is claimed for this follow-up.

## Existing CSV time-series check (2026-10-05)

A read-only check of the 200 numeric rows in the standalone profile above found
that `FrameTime` had a Pearson correlation of 0.688 with `GameThreadTime`, 0.198
with `RenderThreadTime`, -0.168 with `RHIThreadTime`, and 0.314 with `GPUTime`.
`NumInstanceTransformUpdates` had a correlation of -0.154 with `FrameTime`; its
median was 28,193 over all rows versus 28,232 among the 20 slowest frames. Thus,
the large update count remains a plausible steady cost, but its per-frame
variation does not explain the worst-frame variation in this capture. This
single instrumented run is noisy evidence, not a causal profiler result.

The next paired Game-mode capture should retain the sway ablation as a test of
steady cost, but also capture a warmed Game Thread profile around `TickActors`
and representative resident/foliage ticks. Do not prioritize per-frame
transform-update spikes as the explanation for the slowest frames without a
new measurement. The 30 FPS gameplay target and complete habitat-composition
requirement remain unchanged.

## Ground-cover sway A/B (2026-10-05)

A matched, bounded standalone Game-mode pair was run from an isolated UE 5.8.3
scratch project at the same 1600x900 Tideglass midday view. Both captures used
the same 200-frame CSV profile, disabled agent thinking and Python, and created
1,780,040 ground-cover instances. The only test change was the scratch-only
`-IslandDisableGroundCoverSway` diagnostic. That switch skips the periodic
CPU transform updates for ground cover and resident-brush sway; it preserves
instance placement and visibility. It is not enabled in the project source or
by default. `IslandWeather.cpp` in the scratch copy was built successfully
with UE 5.8.3 before the ablation capture.

| Measure | Sway enabled p50 / p95 | Sway disabled p50 / p95 |
|---|---:|---:|
| Frame time | 164.82 / 292.37 ms | 14.89 / 95.16 ms* |
| Game Thread | 141.75 / 273.22 ms | 5.59 / 20.44 ms |
| Render Thread | 145.77 / 175.62 ms | 14.78 / 95.17 ms |
| GPU | 22.40 / 25.61 ms | 12.36 / 18.85 ms |
| `TickActors` exclusive | 102.36 / 137.87 ms | 0.33 / 1.50 ms |
| `STAT_SkinningSceneExtension` exclusive | 93.54 / 119.02 ms | 0.09 / 0.13 ms |

\* The disabled capture had 201 numeric `FrameTime` entries for the 200-frame
capture; p50/p95 are reported from those rows as recorded. Both profiles also
contain extreme hitch outliers, so neither the median nor this short capture
should be read as a sustained gameplay FPS guarantee. The enabled and disabled
captures had comparable GPU-scene population (median 2.31M instances in each),
and the diagnostic log confirms the ground-cover count was unchanged.

The large collapse in `TickActors` and skinning-extension time is strong
evidence that this CPU transform/sway path dominates the measured cost in this
capture. With sway disabled, the median frame time is comfortably below the
33.3 ms 30-FPS budget, but p95 remains 95 ms and therefore still fails the
target under hitchy frames. The visual effect was disabled, so this is a
diagnostic upper bound rather than an acceptable shipped fix. The earlier
correlation check remains relevant: transform-update count did not predict the
slowest frames within the sway-enabled capture, even though removing the whole
update path dramatically improved this paired run.

Next, preserve nearby wind and resident-brush response while eliminating
per-frame CPU updates across the full ground-cover population. Prefer a
material/GPU-driven wind path, or a tightly bounded view-/interaction-local
update set, and validate both in warmed repeated captures. Keep full placement
and visibility, the 30-FPS minimum, and the visual habitat composition as
acceptance criteria. Logs and CSVs are under
`Saved/CompileScratch/Codex_UnderstoryVerify_20261002/Project/Saved/Playtests/Codex_SwayAblation/`
and `Saved/CompileScratch/Codex_UnderstoryVerify_20261002/Project/Saved/Profiling/CSV/`.

### Retained-radius sweep (2026-10-05)

To test whether a smaller active sway neighborhood preserves close-up life while
reducing transform work, two additional standalone Game captures used scratch
builds with normal sway enabled and the same 1600x900 Tideglass midday view.
The only source difference between these trials was the scratch constant for
`FoliageSwayFocusRadius` (1500 cm, then 750 cm); the 3000 cm production source
was not changed. Each CSV contains 200 valid frame rows. Because UBT stalled
before creating its log, the scratch translation unit was compiled with the
cached UE 5.8.3 response files and the scratch module was linked directly; no
project module or content was rebuilt in place.

| Active sway radius | Frame time p50 / p95 | Game Thread p50 / p95 | Render Thread p50 / p95 | `TickActors` p50 / p95 | Transform updates p50 / p95 |
|---:|---:|---:|---:|---:|---:|
| 30 m, baseline | 164.82 / 292.37 ms | 141.75 / 273.22 ms | 145.77 / 175.62 ms | 102.36 / 137.87 ms | 28,391 / 28,392 |
| 15 m | 111.07 / 205.34 ms | 41.15 / 110.49 ms | 103.31 / 170.34 ms | 17.25 / 36.26 ms | 0 / 5,650 |
| 7.5 m | 120.64 / 228.71 ms | 27.92 / 106.56 ms | 132.56 / 177.91 ms | 4.95 / 32.44 ms | 161 / 1,495 |

Both trials retained the same 1.78M ground-cover placements and GPU time stayed
near 22–23 ms p50. The narrower radii reduce `TickActors` and transform-update
counts, but do not approach the 33.3 ms frame-time budget. The 7.5 m capture is
slower overall than the 15 m capture despite fewer updates; this short,
outlier-heavy sample is too variable to rank those radii as production choices.
The sway-disabled run remains much faster, confirming CPU transform updates
are a meaningful cost, but these retained-radius trials show they are not the
only dominant cost. Do not reduce the project default radius based on this
sweep; the close-range visual tradeoff has not earned the change.

The next performance milestone should target the remaining foliage rendering
cost while preserving full population and visibility: audit the number and
layout of HISM components and measure a reversible per-mesh consolidation in a
scratch copy, with repeated warmed captures. Keep the sway radius and visual
behavior unchanged during that structural test so its effect is attributable.
Evidence: `Saved/CompileScratch/Codex_UnderstoryVerify_20261002/Project/Saved/Profiling/CSV/Profile(20261005_143226).csv`
and `Profile(20261005_143929).csv`; logs are in
`Saved/Playtests/Codex_SwayRadius1500/` and `Saved/Playtests/Codex_SwayRadius750/`.
