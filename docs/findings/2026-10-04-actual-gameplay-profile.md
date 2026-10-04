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

The diagnostic code change was not retained: the scratch UBT build waited on
Unreal's shared build mutex before creating its UBT log and was cancelled as
Codex's own waiting shell. Other inaccessible `dotnet` processes were left
untouched. No performance fix, compile, or A/B pass is claimed here.
