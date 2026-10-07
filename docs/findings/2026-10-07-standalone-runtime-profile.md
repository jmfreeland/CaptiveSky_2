# Bounded standalone Game profile (2026-10-07)

## Capture

A standalone UE 5.8.3 Development Game session loaded `/Game/Maps/Island` with
`-Spectator`, a 1600x900 window, and a 120-second realtime limit. The CSV
profiler sampled 600 frames after a 30-second warm-up. Agent thinking and
Python were disabled, and the run ended at 120.2 seconds with zero model
requests. The CSV window spans about 9.47 seconds of engine frame time. This is
a bounded spectator/render-simulation check, not interactive PIE or a packaged
build. The capture does not verify character agency, landmark interactions, or
the 30-FPS floor during user-controlled play.

The host was an Intel i9-13900HX laptop with an RTX 4080 Laptop GPU, D3D12 SM6.
The command requested 1600x900; the log records that resolution when the game
window initializes and when CSV capture begins. The initial startup metadata
briefly reports 1280x720 before window initialization. The run used the
project's local DDC and isolated saved-data root. Shaders and assets were still
cold-starting before the capture window.

CSV: `Saved/Profiling/CSV/Profile(20261007_121229).csv`

Log: `Saved/Logs/Codex_StandaloneRuntimeProfile_20261007.log`

## Measurements

Percentiles are over all 600 numeric CSV frames, in milliseconds.

| Measure | p50 | p95 | Max |
|---|---:|---:|---:|
| Frame time | 15.00 | 19.26 | 51.51 |
| Game thread | 5.24 | 7.14 | 37.82 |
| Render thread | 14.97 | 18.01 | 52.38 |
| GPU | 10.14 | 12.42 | 13.58 |
| RHI thread | 5.19 | 6.11 | 8.50 |
| `TickActors` exclusive | 0.33 | 1.69 | 33.73 |
| `STAT_SkinningSceneExtension` exclusive | 0.11 | 0.15 | 24.74 |
| `RayTracing_FinishGatherInstances` exclusive | 5.52 | 6.99 | 9.37 |
| Instance transform updates | 0 | 0 | 1 |

The sample's p50 and p95 frame times are below 33.3 ms, but its maximum is not;
that is encouraging for this particular spectator view, not proof that the
30-FPS target is met in ordinary play. `RayTracing_FinishGatherInstances` is
the largest listed steady render-thread scope in this capture. `TickActors`
and the skinning-scene-extension scope are small at p50/p95, despite large
outliers. Those counters do not by themselves establish a cause or explain
other sessions.

At shutdown the log contains a `LogCrowdFollowing` warning that no Recast
NavMesh was available while creating a crowd manager. It appeared during
teardown of this spectator run; it needs a separate gameplay-path reproduction
before treating it as a user-visible defect. The process exited normally.

## Interpretation and next check

This is much faster than the 2026-10-04 standalone profile recorded in
[`2026-10-04-actual-gameplay-profile.md`](2026-10-04-actual-gameplay-profile.md)
(262.72 ms p50 / 404.68 ms p95), but the view, runtime state, warm-up, and exact
capture conditions are not matched. The earlier PIE note of 8-13 FPS is also a
different mode and workload. Do not treat this as a performance fix or as
evidence that the earlier gameplay result was spurious.

When the editor/runtime is available, first capture the same camera, resolution,
scalability, and warmed phase as the reported slow PIE case. Compare a normal
resident-play run to a no-thinking diagnostic only if both retain the same
rendered scene and controls. Keep agent calls bounded and record the request
count. If the game path reproduces a frame time above 33.3 ms, profile that
matched path before changing foliage density or disabling wind; retain nearby
vegetation movement and the full habitat while investigating.
