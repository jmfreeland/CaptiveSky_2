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
