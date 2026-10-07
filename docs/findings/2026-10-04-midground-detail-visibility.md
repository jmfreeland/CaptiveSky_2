# Fine vegetation through the near midground

## Why

After excluding shadowless foliage from distance-field lighting (`175096b`),
the captures regained enough headroom to revisit visibility instead of reducing
the population. Fine foliage previously disappeared at 45 m (55 m for cattails
and Phalaris). This left a visibly bare band behind the landmarks even though
the scatter already contains about 1.78 million terrain-filtered meadow instances.

## Matched experiment

UE 5.8.3, private project, same rebuilt DLL and shared map fingerprint:
`3D1F2432A3D01D6E88C9CDF3D1B81FC7AEA312E7B64500F444FB8F1957E10AA8`.
No gameplay, resident turns, map saves or canonical-state writes. A main editor
was active, so this was private-world validation, not hardware-isolated testing.
Each owned capture editor had a four-minute external limit and exited normally.

Opt-in `-IslandFoliageDetailRangeCm=9000` changed only the fine foliage draw range
to 90 m with fade starting at 60 m. Tree range (350/550 m), shrubs/rhododendrons
(80/140 m), population budgets, species selection and scatter positions stayed
unchanged. The private experimental build succeeded in 14.02 seconds.

| Run | Ground-level p95 FPS range | Survey p95 FPS | Result |
| --- | --- | --- | --- |
| Existing ranges | 54.51..57.54 | 47.80 | All nine views passed |
| 90 m fine-foliage preview | 56.04..57.83 | 52.33 | All nine views passed |

Logs: `Saved/Logs/Codex_FoliageRangeBaseline_20261004.log` and
`Saved/Logs/Codex_FoliageRange90m_20261004.log`. Both used 17:00, 1600x900,
10 warmup frames then 50/50 valid intervals per camera. These sequential samples
do not demonstrate that extending range improves performance; timings are within
the variability of these short captures. They do demonstrate passing the existing
30 FPS p95 ground-view gate in both runs.

Compared images in scratch `Saved/Viewpoints/2026-10-04_163823_h17.0` and
`2026-10-04_163925_h17.0`: Tideglass gains a visible green band behind the rock
and bench; the shore view gains some low vegetation. The distant hills remain
bare and the pool/arch are still placeholder geometry. This is a near-midground
improvement, not a claim to have finished the landscape.

## Adopted policy

Fine grass, low ground plants, meadow flowers, Festuca, Phalaris and cattails now
use 60/90 m fade/cull distances by default. No additional instances are spawned.
The bounded command-line preview remains available for process-only comparisons:
`-IslandFoliageDetailRangeCm=<4500..15000>` (centimetres); fade begins at two thirds
of the selected range. Values outside that interval are warned about and ignored,
keeping the default. It does not alter tree or broadleaf-understory ranges.

GroundCover regression expectations pin the new fine-detail defaults while
retaining tree/shrub ranges, species/clearance checks and the distance-field policy
contract. The adoption build succeeded privately (5 actions, 11.28 seconds).
The main editor's DLL remains untouched/locked; a main build and restart are still
needed to load these source changes there.

## Remaining work

Acceptance after promoting the default:

- `Codex_RangeAcceptDefault_20261004.log`: GroundCover passed, no range override.
- `Codex_RangeAcceptInvalid_20261004.log`: GroundCover passed with rejected
  `-IslandFoliageDetailRangeCm=90000`; warning observed and default ranges retained.
- `Codex_RangeAcceptViews_20261004.log`: all nine 17:00 cameras passed, ground-level
  p95 55.41..57.81 FPS, survey 35.64 FPS, each 50/50 valid intervals. Normal exit.
  Output `2026-10-04_164638_h17.0`; inspected Tideglass still shows the green band.
  Meadow population remains 1,780,040 instances.

The shared map changed during later acceptance to SHA256
`FA97753033546EBF8C42B68E6465912CC2DAAF6DF5D07BF541B92333CEDD3FF9` while
the other agent's ocean work was in flight. Thus this last run is current-scene
acceptance, not a controlled performance comparison with the earlier baseline.
Only the first baseline/90 m pair retained the identical map hash.

Investigate distant terrain appearance separately: mesh range alone will not
supply continuous ground texture or useful biome transitions. Keep real-time and
model-request safeguards intact.

## Matched Tideglass Game range comparison (2026-10-07)

`Start-Spectator.ps1` exposes the existing `-IslandFoliageDetailRangeCm`
preview as a validated `-FoliageDetailRangeCm` argument (4,500–15,000 cm).
PowerShell parsing and rejection of an out-of-range value passed. Two earlier
launches did not reach the Island: one could not create shader-transfer files
in the default user-profile directory; another still stalled after platform
validation with a private shader path. Comparing those logs with a successful
same-day Game run identified the proven `-NoZenLocalFallback` plus warmed
`-LocalDataCachePath` setup. With those flags, both matched runs became
world-ready in 14.2–15.4 seconds.

Both standalone Game sessions used UE 5.8.3, 1600×900, the same five repeated
Tideglass viewpoints at 11:00, the same current source/map, 15 seconds of
warm-up, and 600 CSV frames. They disabled resident thinking and Python, used
separate isolated data roots, capped play at 60 seconds and model requests at
one, and exited normally after 60.5 / 60.2 seconds with zero model requests.
The only intentional render change was fine foliage fade/cull range:

| Range | Fine-detail fade/cull | FrameTime p50 / p95 / p99 | >33.3 / >100 ms | Max | p95 result |
|---|---:|---:|---:|---:|---|
| 90 m | 60 / 90 m | 15.85 / 22.56 / 436.39 ms | 18 / 7 of 600 | 466.77 ms | 44.3 FPS; clears 30-FPS screen |
| 150 m | 100 / 150 m | 15.75 / 23.74 / 459.22 ms | 24 / 10 of 600 | 646.61 ms | 42.1 FPS; clears 30-FPS screen |

The 150 m [Game frame](../../Saved/Playtests/Codex_TideglassRange150m_20261007/Screenshots/001_Tideglass.png)
extends grass and low plants behind the central rocks and into much of the
formerly bare midground without adding instances. The [90 m frame](../../Saved/Playtests/Codex_TideglassRange90m_20261007/Screenshots/001_Tideglass.png)
shows the matched baseline. The added band improves this composition, but the
far horizon remains sparse and this one stationary camera does not establish
traversal or target-hardware performance. The 150 m sample's 1.18 ms higher p95
and 646.61 ms maximum are worth carrying forward; sequential captures cannot
attribute either difference to range alone. Both screenshot runs have a
pronounced hitch tail despite passing the p95 screen: p99 is 436–459 ms, with
7 / 10 frames over 100 ms. The worst frames report only 17–18 ms GPU time, but
this does not by itself identify which worker or operating-system delay caused
the hitch. A clean 30-FPS percentile is not a no-hitch guarantee.

Logs: [90 m](../../Saved/Logs/Codex_TideglassRange90m_20261007.log),
[150 m](../../Saved/Logs/Codex_TideglassRange150m_20261007.log). CSV:
[90 m 600-frame profile](../../Saved/Profiling/CSV/Profile%2820261007_211656%29.csv),
[150 m 600-frame profile](../../Saved/Profiling/CSV/Profile%2820261007_211900%29.csv).
The 150 m preview remains opt-in pending a separate PIE traversal and
performance review; no global default or instance budget has been changed.

## Screenshot-free 150 m profile (2026-10-08)

To check whether the screenshot queue was contaminating the long tail, the 150 m
run was repeated with the same five Tideglass viewpoints and the same render,
clock, warm-cache, isolation and runtime/request caps, but without `-Shots`. The
new `-ViewpointHour 11` option applies the fixed hour under an explicit isolated
`-DataRoot` even when screenshots are disabled; the wrapper still refuses to
apply it to the normal world-state directory. The 600-frame capture completed and
the game exited at its 60-second limit with zero model requests.

| 150 m capture | FrameTime p50 / p95 / p99 | >33.3 / >100 ms | Max |
|---|---:|---:|---:|
| Screenshots enabled | 15.75 / 23.74 / 459.22 ms | 24 / 10 of 600 | 646.61 ms |
| Screenshots disabled | 15.58 / 21.03 / 34.70 ms | 14 / 4 of 600 | 474.65 ms |

The large tail reduction is consistent with screenshot capture contributing
substantial stalls in the earlier sample, whose log shows a screenshot queued
near the end of the CSV window. It is not proof that screenshots explain all the
outliers: four frames still exceeded 100 ms, the maximum remained 474.65 ms, and
these were sequential runs on a shared machine. The no-screenshot 150 m sample
still clears 30 FPS at p95, but does not demonstrate hitch-free traversal or
target-hardware performance. A clean, matched 90 m no-screenshot sample is still
needed; its attempted launch stalled in Turnkey platform validation before the
Island became ready, so it produced no comparable capture.

Log: [150 m, screenshots disabled](../../Saved/Logs/Codex_TideglassRange150m_NoShots_20261008.log).
CSV: [150 m, 600-frame screenshot-free profile](../../Saved/Profiling/CSV/Profile%2820261007_213005%29.csv).
