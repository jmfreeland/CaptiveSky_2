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

## 150 m standalone Game comparison attempt (2026-10-07)

`Start-Spectator.ps1` now exposes the already-supported
`-IslandFoliageDetailRangeCm` command-line preview as a validated
`-FoliageDetailRangeCm` argument (4,500–15,000 cm). PowerShell parsing and the
range guard passed. A matched 90 m Game baseline was not obtained in this
restricted session: the first launch could not create shader-transfer files in
the default user-profile shader directory. Retrying with a private
`Saved/Playtests/.../ShaderWorking` directory passed that point, but UE did not
reach the world-ready marker within the 240-second startup guard; it stopped
after launching `ValidatePlatforms`. The launcher ended only that Game process.

Both attempts disabled resident thinking/Python, used an isolated data root, a
60-second play cap and a one-request ceiling; no model request, Island save,
screenshot or CSV profile was produced. Logs:
[`first shader-path failure`](../../Saved/Logs/Codex_TideglassRange90m_20261007-backup-2026.10.07-20.07.42.log)
and [`private shader-path startup timeout`](../../Saved/Logs/Codex_TideglassRange90m_20261007.log).
This establishes a launch-environment issue, not a foliage-range result. The
150 m comparison remains untested; resume only when Game startup can reach the
Island within the bounded guard.
