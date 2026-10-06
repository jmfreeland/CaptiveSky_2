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
cost while preserving full population and visibility. A component-count audit
below found only 22 HISM groups, so consolidation is not the leading hypothesis.
Keep the sway radius and visual behavior unchanged while measuring the dominant
species/layer cost in the same real Game view; use only scratch visibility
ablations for diagnosis, then optimize the measured group without reducing the
full-scene composition.
Evidence: `Saved/CompileScratch/Codex_UnderstoryVerify_20261002/Project/Saved/Profiling/CSV/Profile(20261005_143226).csv`
and `Profile(20261005_143929).csv`; logs are in
`Saved/Playtests/Codex_SwayRadius1500/` and `Saved/Playtests/Codex_SwayRadius750/`.

### Runtime HISM population audit (2026-10-05)

A one-shot diagnostic in the isolated scratch `AIslandWeather::Tick` enumerated
every world actor and its HISM components after the Island had initialized.
The standalone Game log reports 22 nonempty HISM components and 1,812,286
instances total; all 22 components belong to `IslandWeather_0`. The apparent
`~120+ HISM` count in the earlier handoff note is not borne out by this direct
world audit. The ground-cover actor already batches one HISM component per
species/mesh group, so there is little evidenced component-consolidation
headroom.

Three grass meshes account for 982,791 instances (54.2%); five ground-plant
meshes account for 797,779 (44.0%). Together these eight components represent
98.2% of the total. Trees, shrubs, rhododendrons, flowers, and wetland accents
make up the remaining 1.8%. `ground_05_01` is the sole duplicated mesh path, in
`ShoreGroundPlants` and `IslandShrubs`; their different cull/interaction roles
make merging them an unattractive first test. The audited counts match the
current code's 22 transient ground-cover components, and no primary Content or
map assets were changed.

Log: `Saved/Playtests/Codex_HismAudit_20261005/HismAudit_Game.log`. The
diagnostic code was scratch-only, the 45-second Game session exited normally,
and no Unreal Editor/Game/Live Coding process remained afterward. The direct
population evidence changes the next step: profile the three grass and five
ground-plant groups in the real Game path before attempting mesh or component
consolidation.

### Real-Game foliage visibility ablation (2026-10-05)

Using the same scratch module, 1600x900 Tideglass camera, and 200-frame CSV
capture, I hid HISM groups only for diagnosis and disabled
CPU-driven sway in every condition. All 22 components and all 1,812,286
instances remained allocated; only visibility changed. Two all-visible runs
provide a repeatability check, while the group and species runs isolate render
cost without conflating it with the transform-update result above.

| Visible HISM groups | Instances visible | Frame time p50 / p95 | Render Thread p50 / p95 | GPU p50 / p95 |
|---|---:|---:|---:|---:|
| All, repeat 1 | 1,812,286 | 14.89 / 95.16 ms | 14.78 / 95.17 ms | 12.36 / 18.85 ms |
| All, repeat 2 | 1,812,286 | 14.63 / 95.48 ms | 14.54 / 96.08 ms | 12.32 / 17.23 ms |
| Three grass groups | 982,791 | 12.95 / 95.39 ms | 12.92 / 95.45 ms | 10.35 / 16.52 ms |
| Five broadleaf ground-plant groups | 797,779 | 11.94 / 48.69 ms | 11.98 / 48.78 ms | 9.11 / 14.21 ms |
| Remaining 14 groups | 31,716 | 10.43 / 28.98 ms | 10.46 / 40.30 ms | 8.17 / 12.91 ms |

The three grass species were then isolated individually:

| Grass component / mesh | Instances | Frame time p50 / p95 | GPU p50 / p95 |
|---|---:|---:|---:|
| `ShoreGrassA` / `grass_01_02_mesh` | 326,303 | 10.86 / 40.92 ms | 8.76 / 13.18 ms |
| `ShoreGrassB` / `grass_01_03_mesh` | 324,716 | 10.62 / 35.23 ms | 8.56 / 13.35 ms |
| `ShoreGrassC` / `grass_01_04_mesh` | 331,772 | 10.88 / 42.71 ms | 8.65 / 13.54 ms |

The two all-visible sway-disabled runs reproduce the same ~95 ms p95 tail.
Grass alone nearly reproduces it; broadleaf ground plants alone have a smaller
tail, and the remaining vegetation alone is below the 33.3 ms 30-FPS frame
budget in this short sample. No individual grass species explains the whole
grass-group result: the combined three are materially worse at p95 than any
one alone. With GPU p95 below 18 ms even in the full scene, this points to
CPU/render-thread work or interaction among the dense grass HISM groups, not a
GPU-throughput limit by itself. It does not yet identify a specific engine
scope or prove that grass removal is an acceptable solution.

All visibility variants retained full placement, spacing, collision and
navigation settings; the hidden layers are not a candidate production state.
The first Grass-B process stalled in Unreal platform-validation startup before
loading the map; only that scratch editor process and its matching UBT child
were stopped. A fresh retry completed, as did Grass C. All completed capture
logs are under `Saved/Playtests/Codex_HismSolo_20261005/`; the CSVs are
`Profile(20261005_141620).csv`, `Profile(20261005_145521).csv`,
`Profile(20261005_145700).csv`, `Profile(20261005_145835).csv`,
`Profile(20261005_150112).csv`, `Profile(20261005_150507).csv`,
`Profile(20261005_151253).csv`, and `Profile(20261005_151518).csv` in the
scratch profiling directory. The next implementation should preserve the full
grass silhouette and investigate why three individually tolerable groups
produce a high combined render-thread tail; do not remove or thin a species
without an attributable optimization and a full-composition visual review.

### Runtime mesh LOD and material capability audit (2026-10-05)

A second bounded scratch Game run logged each populated HISM mesh's static LOD
triangle counts, Nanite-enabled flag, and assigned materials' world-position-
offset (WPO) capability. It completed the Island load and normal 45-second
runtime cap with exit code 0. The audit did not change production assets,
placement, or visibility. Full output: `Saved/Playtests/Codex_GrassAssetAudit_20261005/GrassAssetAudit_Game.log`.

| Dominant group | Instances | LOD0→LOD3 triangles | Nanite | Any assigned material uses WPO |
|---|---:|---|:---:|:---:|
| `ShoreGrassA` / `grass_01_02_mesh` | 326,303 | 774→387→194→97 | yes | yes |
| `ShoreGrassB` / `grass_01_03_mesh` | 324,716 | 1,200→600→300→150 | yes | no |
| `ShoreGrassC` / `grass_01_04_mesh` | 331,772 | 422→211→106→64 | yes | yes |
| `ShoreGroundPlants` / `ground_05_01` | 160,398 | 208→104→63→63 | yes | yes |
| `ShoreGroundPlantLowA` / `ground_01_01` | 157,780 | 46→46→46→46 | yes | yes |
| `ShoreGroundPlantLowB` / `ground_01_02` | 162,244 | 46→46→46→46 | yes | yes |
| `ShoreGroundPlantLowC` / `ground_06_01` | 157,126 | 137→69→63→63 | no | yes |
| `ShoreGroundPlantLowD` / `ground_12_01` | 160,231 | 362→181→91→63 | no | yes |

The grass ablation's CPU-sway switch does not disable these material WPO paths:
two grass materials and all five broadleaf groups still report WPO-capable
materials. This is a useful separation to preserve in future tests; the prior
GPU p95 remained under 18.9 ms, so the capability flag alone does not establish
WPO as the frame-time bottleneck. Conversely, every grass mesh already has
Nanite enabled, so “enable Nanite on grass” is not a viable next change. The
two non-Nanite broadleaf meshes are not evidence for a production conversion;
the earlier broader Nanite foliage experiment regressed the full scene.

Static LOD0 triangle count also does not explain the isolated grass results:
Grass B has the highest count but the lowest single-group p95, while Grass C
has the lowest count and the highest p95. The runtime audit enumerates authored
LOD data, not which Nanite clusters or instances contributed to each captured
frame. Therefore neither a triangle-driven mesh simplification nor a blanket
Nanite change is justified. Keep the full composition and investigate the
combined HISM/Nanite instance-culling and render-thread path with a frame trace
that attributes work to the three grass groups; any candidate must then be
retested in the same all-visible Game view against the 30-FPS p95 goal.

### CPU foliage sway and scene-culling cost (2026-10-05)

The all-visible frame trace now attributes the large CPU cost to the animated
instance-transform path, not static mesh triangle count alone. In the scratch
copy, the production `FoliageSwayFocusRadius` was set to 3000 cm, matching the
current main-source value, and the update interval remained 0.1 seconds. A
matched trace-only diagnostic at 750 cm was also run, as was a 750 cm run with
the scratch-only `-IslandDisableGroundCoverSway` flag. All kept the same
1600x900 camera, 22 HISM groups and 1,812,286 instances; each Game session was
capped at 45 seconds and made zero model requests.

Across the 25–55 second stable trace window, the aggregated worker timer
`SceneCulling_Post_UpdateInstances` averaged 59.28 ms per update at 750 cm
(139 updates) and 124.98 ms at 3000 cm (82 updates). With CPU transform sway
disabled, it averaged 0.75 ms (1,252 updates). On the render thread,
`SceneCulling_Update_FinalizeAndClear` averaged 64.71 ms per update at 750 cm
and 137.18 ms at 3000 cm; the observed maximum `WaitForVisibilityTasks` was
125.7 ms at 750 cm. These are traced inclusive scope timings, so worker values
must not be summed as frame latency; the comparison is for identifying the
workload that scales with CPU sway. Trace instrumentation also adds substantial
overhead, so its frame-rate numbers are not used as gameplay benchmarks.

A separate untraced 200-frame CSV at 750 cm completed in the same bounded Game
run. It measured frame-time p50/p95 of 142.65/251.88 ms, Render Thread
151.87/194.72 ms, and GPU 18.09/20.88 ms. The earlier all-visible,
CPU-sway-disabled untraced pair was 14.63/95.48 ms frame p50/p95. This confirms
that the 750 cm CPU-sway candidate is not a viable production setting and that
CPU instance transforms are a major added cost; the 3000 cm default is worse
in the matched trace. Neither radius meets the 30-FPS p95 target, and no
vegetation or wind behavior was changed in the primary project.

Next optimization should retain wind-driven motion and close resident/character
brush response while avoiding repeated `UpdateInstanceTransform` calls across a
large camera radius. Verify that the existing WPO-capable materials visibly
provide ambient wind, then prototype a much smaller CPU interaction footprint
in scratch and compare both motion quality and untraced 200-frame p50/p95.
Do not simply ship the sway-off diagnostic or adopt 750 cm without a visual
review; preserve full population and require the full all-visible p95 to move
toward 33.3 ms.

Evidence: `Saved/Playtests/Codex_GrassRenderTrace_20261005/` (the three `.utrace`
files, per-thread Insights exports, and bounded Game logs) and
`Saved/CompileScratch/Codex_UnderstoryVerify_20261002/Project/Saved/Profiling/CSV/Profile(20261005_160449).csv`.

### Strictly local 180 cm sway profile (2026-10-05)

A follow-up untraced 200-frame capture used the same all-visible 1600x900
Tideglass midday Game view, with the scratch-only `FoliageSwayFocusRadius`
reduced to 180 cm. The full 1,812,286-instance population remained in place,
and the run completed normally within its 45-second real-time cap. This radius
matches the existing close resident-brush interaction scale, but it was applied
to the scratch CPU-sway path only; the production source remains unchanged.

| Measure | 180 cm p50 / p95 | 750 cm p50 / p95 |
|---|---:|---:|
| Frame time | 14.12 / 129.57 ms | 142.65 / 251.88 ms |
| Game Thread | 5.66 / 37.85 ms | — |
| Render Thread | 13.89 / 112.73 ms | 151.87 / 194.72 ms |
| GPU | 10.36 / 15.43 ms | 18.09 / 20.88 ms |

At 180 cm, median frame time is near the sway-disabled median and dramatically
better than the 750 cm condition, while preserving immediate CPU interaction
near the camera. Its 129.57 ms p95 still misses the 33.3 ms 30-FPS target, so
this is a promising interaction radius, not a complete performance fix. The
high p95 tail also means the median alone is not evidence of a shippable result.
No visual comparison was captured, and the presence of WPO-capable materials
does not establish that ambient wind remains visibly convincing when the CPU
radius is this small. Review the full habitat at this setting, verify the wind
motion in-game, and profile the remaining hitch tail before changing production.

Evidence: `Saved/CompileScratch/Codex_UnderstoryVerify_20261002/Project/Saved/Profiling/CSV/Profile(20261005_161032).csv` and
`Saved/Playtests/Codex_GrassSway180_Untraced_20261005/`.

### Main-tree local-sway capture (2026-10-06)

The main-tree source now exposes `CaptiveSky.Island.FoliageSwayFocusRadiusCm` for
bounded capture-time experiments. Its production default remains 3000 cm; the
launcher override set 180 cm only for this test. A 200-frame standalone Game
capture retained all 1,779,877 ground-cover placements at the same Tideglass
view with agent thinking and Python disabled. After dropping the first 20
warm-up rows, the remaining 180 rows measured:

| Measure | p50 / p95 |
|---|---:|
| Frame time | 17.30 / 138.93 ms |
| Game Thread | 6.12 / 28.48 ms |
| Render Thread | 15.74 / 124.74 ms |
| GPU | 14.59 / 19.02 ms |
| `TickActors` exclusive | 0.51 / 16.13 ms |
| `NumInstanceTransformUpdates` | 0 / 26 |

This reproduces the low median of the earlier scratch-only 180 cm trial, but
the 138.93 ms p95 still falls far short of the 33.3 ms budget for a 30-FPS
minimum. The first screenshot was also taken during a 193.65 ms startup hitch;
the next view showed 41.46 ms. Keep the 3000 cm production behavior unchanged,
and treat 180 cm as an experiment rather than a performance fix. The capture
demonstrates that substantially reducing CPU updates alone does not eliminate
the render-thread hitch tail; next inspect a warmed frame trace for the source
of those remaining stalls while preserving the full ground-cover population
and nearby character response.

Evidence: `Saved/Profiling/CSV/Profile(20261006_060409).csv`,
`Saved/Logs/Codex_FoliageSwayRadius180_20261006.log`, and
`Playtests/Codex_FoliageSwayRadius180_20261006/Screenshots/002_Tideglass.png`.

### Delayed steady-state sample (2026-10-06)

The spectator now has an opt-in delayed CSV capture so a fixed Game-mode view can
settle before measurement without console input: pass both
`-CSVProfileFrames 200` and `-CSVProfileDelaySeconds 45` to
`Scripts/Start-Spectator.ps1`. The delay uses real elapsed time from spectator
startup; the existing immediate-capture option remains unchanged. The
`CaptiveSky_2Editor` target built successfully, and the standalone Game log
confirms the delayed command was accepted at 45.0 seconds and wrote exactly 200
frames in 8.23 seconds.

This repeated the same 1600x900 Tideglass view with the capture-only 180 cm sway
radius, agent thinking/Python disabled, and the full vegetation population
intact. After omitting the first 20 CSV rows for comparison with the prior
profiles, the remaining 180 rows measured:

| Measure | p50 | p95 | Max |
|---|---:|---:|---:|
| Frame time | 17.63 ms | 144.07 ms | 167.13 ms |
| Game Thread | 5.99 ms | 30.74 ms | 54.83 ms |
| Render Thread | 15.63 ms | 118.93 ms | 144.27 ms |
| GPU | 14.77 ms | 20.57 ms | 21.43 ms |
| `STAT_SkinningSceneExtension` | 0.08 ms | 67.00 ms | 98.71 ms |
| Render-thread `RDG` | 0.41 ms | 54.62 ms | 57.91 ms |
| Render-thread `RenderOther` | 2.54 ms | 37.98 ms | 55.06 ms |
| `NumInstanceTransformUpdates` | 0 | 23 | 24 |

The capture reports zero ordinary PSO misses in the measured rows (the two
PSO-on-hitch counters are unavailable and read -1). This weakens, but does not
fully rule out, PSO compilation as the cause of the tail. More importantly, the
delayed sample reproduces the prior result: 180 cm sway is not a performance
fix, and its 144 ms p95 is over four times the 33.3 ms frame budget for the
30-FPS minimum. Render Thread time dominates while GPU p95 remains near 21 ms;
the named skinning and RDG scopes are leads, not proof of a single cause. Do not
change the production 3000 cm radius or remove vegetation on this evidence.

Next, capture a short CPU/GPU Unreal Insights trace around a delayed sample to
attribute the render-thread spikes before changing the full-composition
foliage/wind path.

Evidence: `Saved/Profiling/CSV/Profile(20261006_064724).csv`,
`Saved/Logs/Codex_FoliageDelayedCSV_20261006.log`, and
`Playtests/Codex_FoliageDelayedCSV_20261006/Screenshots/001_Tideglass.png`.

### Warmed Game-mode CPU/GPU trace (2026-10-06)

A delayed 30-second Unreal Insights trace was captured from the same 1600x900
Tideglass standalone Game view, after 45 seconds of settling, with the full
1,779,877 ground-cover placements and other vegetation still present. Agent
thinking and Python were disabled. This was a warmed repeat without the
force-memory DDC option; no ShaderCompileWorker processes were active during
the capture. The session ended normally at its 120.1-second safety cap with
zero model requests. This removes the shader-compile contention that affected
the preceding retry, but it is still one instrumented run rather than a
repeatable benchmark.

The 30-second trace exported about 930 frame events per track. CPU and render
frame medians were near 16 ms, but their p95s remained around 108–111 ms; the
GPU frame-track p95 was 102.91 ms. Roughly 240–250 of 930 frames per track
exceeded 33.3 ms. These frame-track intervals are not a measurement of GPU
busy time, and the trace is not directly interchangeable with the earlier
untraced CSV, but the long-tail hitch is plainly still present after warm-up.

| Frame track | p50 | p95 | p99 | Max | >33.3 ms | >100 ms |
|---|---:|---:|---:|---:|---:|---:|
| Game Thread | 15.89 ms | 111.30 ms | 129.15 ms | 160.58 ms | 250 / 932 | 107 / 932 |
| Render Thread | 15.64 ms | 108.36 ms | 115.30 ms | 160.44 ms | 240 / 935 | 119 / 935 |
| GPU frame track | 15.81 ms | 102.91 ms | 110.02 ms | 157.30 ms | 240 / 931 | 73 / 931 |

The strongest per-frame CPU leads were Lumen scene primitive updates (149.89 ms
maximum for `FDeferredShadingSceneRenderer_BeginUpdateLumenSceneTasks`,
147.52 ms for `UpdateLumenScenePrimitives`), render-thread skinning view data
(89.58 ms), and scene-culling/GPU-scene instance work (about 79–89 ms maxima).
These inclusive/task timings overlap and must not be added together; they
identify what to isolate next, not a proven single cause. The new
`IslandSeaState_ApplySea`, `IslandSeaState_RecomputeWaves`, and
`IslandSeaState_RebuildGPUData` trace scopes each ran once during the sample;
the first two took 0.005 ms and 0.004 ms respectively. That is too little to
explain the observed hitch tail in this run, though one sample cannot rule out
every weather transition.

Next, repeat this delayed capture with only dynamic global illumination disabled
at launch, leaving vegetation, camera, resolution, and other settings intact.
Compare frame-track tails and the Lumen/scene-culling scopes. Keep that override
diagnostic-only: prior PIE tests found no material Lumen FPS change, and any
lighting tradeoff needs a visual review before it could be considered for the
world. If Lumen does not materially shorten the tail, inspect the timing around
the individual scene-culling/skinning tasks rather than reducing habitat
population.

Evidence: `Saved/Logs/Codex_SeaTraceWarm_20261006.log`,
`Saved/Profiling/Traces/Codex_SeaTraceWarm_20261006.utrace`, and the Insights
exports under the current user's temporary directory.

### Matched dynamic-GI diagnostic (2026-10-06)

A second 30-second Insights frame window repeated the same standalone Game view,
camera, resolution, 180 cm capture-only foliage sway radius, full
1,779,877-instance ground-cover population, 45-second settle delay, and warmed
DDC. The only runtime setting changed was
`r.DynamicGlobalIlluminationMethod=0`, applied by the new opt-in
`-DisableDynamicGlobalIllumination` flag in `Start-Spectator.ps1`. The log
confirms the CVar was set to zero; project defaults were not changed. Agent
thinking/Python were disabled and the run exited normally at 120.1 seconds with
zero model requests. The selected frame-event tracks each span 30.0 seconds.

An earlier GI-off pilot used the production 3000 cm sway radius rather than
180 cm, so it is excluded from the matched comparison below.

| Frame track | GI | p50 | p95 | p99 | Max | >33.3 ms | >100 ms |
|---|---|---:|---:|---:|---:|---:|---:|
| Game Thread | On | 15.89 ms | 111.30 ms | 129.15 ms | 160.58 ms | 250 / 932 | 107 / 932 |
| Game Thread | Off | 15.73 ms | 59.20 ms | 79.22 ms | 478.16 ms | 228 / 1141 | 11 / 1141 |
| Render Thread | On | 15.64 ms | 108.36 ms | 115.30 ms | 160.44 ms | 240 / 935 | 119 / 935 |
| Render Thread | Off | 14.76 ms | 61.84 ms | 85.78 ms | 471.24 ms | 228 / 1142 | 11 / 1142 |
| GPU frame track | On | 15.81 ms | 102.91 ms | 110.02 ms | 157.30 ms | 240 / 931 | 73 / 931 |
| GPU frame track | Off | 14.85 ms | 59.56 ms | 78.06 ms | 471.86 ms | 223 / 1141 | 11 / 1141 |

The GI-off p95 is about 43–46% lower and >100 ms frames fell sharply, while
the p50 barely changed. In the GI-on trace, Lumen scene primitive update scopes
reached 149.89 ms (`BeginUpdateLumenSceneTasks`) and 147.52 ms
(`UpdateLumenScenePrimitives`); those Lumen update timers were absent in the
off trace. This is strong evidence that dynamic-GI scene updates account for a
large part of the long hitch tail in this capture, not proof that turning off
GI is a shippable fix. The off run still misses the 33.3 ms p95 budget and has
one ~478 ms outlier; skinning and scene-culling/GPU-scene work also remain
visible (about 68–73 ms maxima in the off trace).

The off screenshot looks flatter and patchier, but the captures used separate
isolated world-state roots, so procedural scatter and lighting are not a clean
visual A/B. Review the screenshots and repeat with an identical deterministic
world-state seed before drawing an appearance conclusion. Keep dynamic GI on in
the project. Next, investigate ways to reduce Lumen's per-frame primitive
update churn while retaining its indirect lighting, then compare repeated
on/off samples and the same seeded scenic view against both the 30-FPS p95
target and full habitat presentation.

Evidence: `Saved/Logs/Codex_LumenOffTrace180_20261006.log`,
`Saved/Profiling/Traces/Codex_LumenOffTrace180_20261006.utrace`,
`Playtests/Codex_SeaTraceWarm_20261006/Screenshots/001_Tideglass.png`, and
`Playtests/Codex_LumenOffTrace180_20261006/Screenshots/001_Tideglass.png`.

### Lumen-on CPU-sway ablation (2026-10-06)

A third delayed 30-second trace kept dynamic GI on and the full 1,779,877
ground-cover instances visible, but used the existing capture-only
`-IslandDisableGroundCoverSway` diagnostic. That skips periodic CPU instance
transform updates for ground-cover and resident-brush sway without removing or
hiding the plants. The camera, 1600x900 resolution, 180 cm radius, 45-second
settle, warmed DDC, 120-second play cap, and zero-request safety settings were
otherwise held constant. The selected frame tracks span 30.0 seconds.

| Frame track | p50 | p95 | p99 | Max | >33.3 ms | >100 ms |
|---|---:|---:|---:|---:|---:|---:|
| Game Thread | 14.57 ms | 17.61 ms | 454.95 ms | 483.85 ms | 18 / 1504 | 18 / 1504 |
| Render Thread | 14.25 ms | 19.00 ms | 454.35 ms | 479.92 ms | 18 / 1508 | 18 / 1508 |
| GPU frame track | 13.55 ms | 19.15 ms | 453.28 ms | 479.78 ms | 18 / 1505 | 18 / 1505 |

With Lumen still enabled, the maximum `UpdateLumenScenePrimitives` scope fell
from 147.52 ms to 0.228 ms; `BeginUpdateLumenSceneTasks` fell from 149.89 ms to
6.11 ms. Scene-culling and GPU-scene maxima fell from roughly 79–89 ms to
0.13–0.26 ms. The Game/Render/GPU p95s were under 20 ms, unlike either the
Lumen-on sway-active trace (~103–111 ms) or the matched Lumen-off trace
(~59–62 ms). This strongly implicates the periodic CPU instance-transform
updates in the expensive Lumen/scene-culling workload, and shows that removing
GI is not necessary to reach the p95 budget in this diagnostic.

However, 18 frames (~1.2%) still had ~0.45–0.48 second stalls. In this
sway-off/Lumen-on baseline, event-level exports show 17 of those 18 frames
overlapping all three `WaitForVisibilityTasks`,
`RHIGetRenderQueryResult_GPU_Wait`, and
`GPUBound_WaitingForGPUForOcclusionQueries_SeeGPUTrack`; `GameThreadWaitForTask`
overlaps all 18. The controlled query-off ablation below removes the
query/visibility waits but leaves most of the long frame stalls, so this is not
the remaining root cause by itself. These captures do not establish a
production 30-FPS guarantee. Keep the full population and Lumen on; next trace
the remaining `GameThreadWaitForTask` / task synchronization path, then
prototype GPU/material wind for ambient motion while preserving the small
near-character response. Verify plant motion and the full habitat visually
before considering any production sway change.

Evidence: `Saved/Logs/Codex_SwayOffLumenOn_20261006.log`,
`Saved/Profiling/Traces/Codex_SwayOffLumenOn_20261006.utrace`, and the Insights
exports under the current user's temporary directory; aligned wait-event
exports confirmed the intermittent occlusion/visibility overlaps.

### Matched occlusion-query ablation (2026-10-06)

Added the capture-only `-DisableOcclusionQueries` option to
`Scripts/Start-Spectator.ps1`. It sends `r.AllowOcclusionQueries 0` through
`-ExecCmds`; the UE 5.8.3 `RendererSettings.h` source confirms that CVar
controls hardware occlusion queries. No project default was changed.

This 30-second Insights window used the same 1600x900 Tideglass viewpoint,
Lumen-on state, 180 cm focus-radius setting, full ground-cover population,
45-second in-world settle, and CPU-sway-disabled diagnostic as the previous
trace. Agent thinking and Python were disabled. The runtime log confirms
`r.AllowOcclusionQueries = "false"`; the session ended normally at its
120.2-second safety cap with zero model requests. This was the first launch
after a cold shader cache, which made startup unusually long; shader-compile
workers were no longer present before the measured window. Separate DataRoots
mean the procedural placements were not seed-identical, so treat this as a
matched-settings diagnostic rather than a deterministic image A/B.

| Frame track | Queries | p50 | p95 | p99 | Max | >33.3 ms | >100 ms |
|---|---|---:|---:|---:|---:|---:|---:|
| Game | On | 14.57 ms | 17.61 ms | 454.95 ms | 483.85 ms | 18 / 1504 | 18 / 1504 |
| Game | Off | 14.64 ms | 16.94 ms | 23.49 ms | 479.61 ms | 14 / 1617 | 14 / 1617 |
| Render | On | 14.25 ms | 19.00 ms | 454.35 ms | 479.92 ms | 18 / 1508 | 18 / 1508 |
| Render | Off | 14.65 ms | 17.04 ms | 35.47 ms | 479.50 ms | 20 / 1617 | 14 / 1617 |
| GPU frame track | On | 13.55 ms | 19.15 ms | 453.28 ms | 479.78 ms | 18 / 1505 | 18 / 1505 |
| GPU frame track | Off | 13.65 ms | 19.21 ms | 21.92 ms | 475.44 ms | 14 / 1618 | 14 / 1618 |

The query-specific `RHIGetRenderQueryResult_GPU_Wait` and
`GPUBound_WaitingForGPUForOcclusionQueries_SeeGPUTrack` events present in the
baseline do not appear in the query-off window. `WaitForVisibilityTasks` fell
from a 474.04 ms maximum (10.63 ms average) to 0.58 ms maximum (0.016 ms
average). That confirms the switch isolates and removes those waits, but the
overall p95 is essentially unchanged and 14 frames still stall for 0.46–0.48
seconds. `GameThreadWaitForTask` remains at 476.71 ms max; the trace also shows
`FTaskBase::WaitWithNamedThreadsSupport` / `SyncPoint_Wait` reaching ~453 ms.
The next diagnosis should follow that remaining synchronization path. Do not
disable occlusion queries in production on this result: it removes one wait
class but does not fix the hitch tail and may increase rendering work.

Evidence: `Saved/Logs/Codex_OcclusionOff_SwayOff_20261006.log`,
`Saved/Profiling/Traces/Codex_OcclusionOff_SwayOff_20261006.utrace`, and the
frame/wait exports in that trace directory; the baseline exports are under the
current user's temporary directory.

### Matched RHI-thread ablation (capture-only; 2026-10-06)

To follow the remaining `GameThreadWaitForTask` path, added the diagnostic
`-DisableRHIThread` switch to `Scripts/Start-Spectator.ps1`. It appends
`r.RHIThread.Enable 0` to the launch `-ExecCmds`; the runtime log confirms the
RHI thread was disabled. This is a capture-only option, not a project default.

The first RHI-off capture included screenshots, and its largest hitch aligned
with `ScreenshotTracing_Execute`/PNG compression. It is excluded from this
comparison. Four screenshot-free captures (two per setting) used the same
Tideglass camera, 1600x900 resolution, Lumen-on scene, 180 cm sway radius, full
ground cover, disabled CPU sway and occlusion queries, 45-second settle, and
30-second trace. All runs reached the 120-second safety cap with zero model
requests. Their DataRoots were separate, so this is a matched-settings
diagnostic rather than a seed-identical A/B.

| Frame track | Capture | RHI thread | p50 | p95 | p99 | Max | >33.3 ms | >100 ms |
|---|---|---|---:|---:|---:|---:|---:|---:|
| Game | Initial | On | 15.28 ms | 17.90 ms | 27.02 ms | 481.85 ms | 11 / 1620 | 11 / 1620 |
| Game | Initial | Off | 21.49 ms | 24.93 ms | 26.37 ms | 38.50 ms | 1 / 1385 | 0 / 1385 |
| Game | Repeat | On | 14.69 ms | 16.69 ms | 22.23 ms | 478.15 ms | 11 / 1689 | 11 / 1689 |
| Game | Repeat | Off | 20.85 ms | 23.89 ms | 25.93 ms | 444.57 ms | 2 / 1409 | 1 / 1409 |
| Render | Initial | On | 15.32 ms | 18.13 ms | 34.83 ms | 478.24 ms | 17 / 1620 | 11 / 1620 |
| Render | Initial | Off | 21.50 ms | 24.93 ms | 26.44 ms | 37.80 ms | 1 / 1385 | 0 / 1385 |
| Render | Repeat | On | 14.70 ms | 17.51 ms | 33.42 ms | 475.90 ms | 17 / 1689 | 11 / 1689 |
| Render | Repeat | Off | 20.87 ms | 23.89 ms | 25.67 ms | 444.62 ms | 2 / 1409 | 1 / 1409 |
| GPU frame track | Initial | On | 14.21 ms | 19.58 ms | 21.29 ms | 476.99 ms | 11 / 1621 | 11 / 1621 |
| GPU frame track | Initial | Off | 22.09 ms | 26.61 ms | 28.30 ms | 31.87 ms | 0 / 1387 | 0 / 1387 |
| GPU frame track | Repeat | On | 13.69 ms | 19.14 ms | 20.14 ms | 470.82 ms | 11 / 1690 | 11 / 1690 |
| GPU frame track | Repeat | Off | 21.24 ms | 25.47 ms | 27.28 ms | 452.06 ms | 1 / 1410 | 1 / 1410 |

Both RHI-on traces reproduced 11 frames above 100 ms in the 30-second window,
with maxima around 0.48 seconds. The initial RHI-off run had none, but its repeat
had one ~445 ms frame: `GameThreadWaitForTask`/`Sync_RenderingThread` reached
~440 ms, and `SyncPoint_Wait` reached ~386 ms. The repeat hitch overlapped a
`FNodeClassRegistry::RegisterNodeInternal` scope, so it may be a separate
one-off initialization event; it proves that disabling the RHI thread does not
eliminate every long synchronization stall. `WaitForVisibilityTasks` remained
short (under 2 ms) in both RHI-off traces. Across both pairs, RHI-off increased
the steady-state p50 by about 6–8 ms and p95 by about 6–8 ms, while the tail
event count was lower but not zero. Two runs per setting are still a small,
seed-varying sample. This is not a production fix or a 30-FPS guarantee; keep
the RHI thread enabled in production. Next, repeat longer and identify the
remaining task/sync-point stall before changing runtime defaults.

Evidence: `Saved/Logs/Codex_RHIThreadOnNoShots_SwayOff_20261006.log`,
`Saved/Logs/Codex_RHIThreadOffNoShots_SwayOff_20261006.log`, the matching
`.utrace` files under `Saved/Profiling/Traces/`, plus repeat logs
`Saved/Logs/Codex_RHIThreadOffNoShotsRepeat2_20261006.log` and
`Saved/Logs/Codex_RHIThreadOnNoShotsRepeat_20261006.log`. Their `.utrace` files
and per-thread, frame, timer, and wait-event CSV exports are alongside the
initial traces.
