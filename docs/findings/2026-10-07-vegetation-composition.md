# Daylight vegetation composition and runtime sample (2026-10-07)

## Bounded gameplay-scale captures

UE 5.8.3 loaded the current Island map in a standalone Game/spectator session at
11:00, using a fresh isolated world-state root, disabled agent thinking and
Python, a one-request ceiling, and a 240-second real-time cap. The run ended
normally after 240.1 seconds with zero model requests. It captured all nine
configured establishing views plus a repeat Shore Approach frame at 1600×900.
The screenshot sequence is under
`Playtests/Codex_Highlight11_Warm_20261007/Screenshots/`.

The representative [Tideglass view](../../Playtests/Codex_Highlight11_Warm_20261007/Screenshots/005_Tideglass.png)
shows the shallow pool, cattails, mixed ground cover, a grazing stag, the inn
edge, and the raven together. Its water and wildlife are legible, but near-field
plants crowd the bottom of the frame. The [ground-detail view](../../Playtests/Codex_Highlight11_Warm_20261007/Screenshots/006_Tideglass_Ground_Detail.png)
shows just how much of a low camera can become foliage. The [Wind Arch view](../../Playtests/Codex_Highlight11_Warm_20261007/Screenshots/008_Wind_Arch_Overlook.png)
is framed by very large dark foreground forms that obscure the shelter and
resident scale. These are composition observations, not proof that plants
block player movement.

Runtime placement reported 1,779,877 ground-cover instances, 10 nonblocking
Tideglass cattails, 359 meadow Fab flowers across 512 sampled sites, and 500
woodland groves containing 14,727 spruce, 15,063 broadleaf shrubs, and 1,487
rhododendrons. This confirms the Island already has abundant low cover and
substantial tree/shrub populations. The broad view still reads as open brown
slope with scattered tree silhouettes, while the Tideglass foreground can feel
overgrown. The evidence points to placement/composition and scale transitions,
not a need to raise the global instance budget immediately.

## Warm screenshot-free 1080p sample

A separate 1920×1080 standalone Game run used the same Island and spectator
views, a 60-second warm-up, no screenshots, disabled agent thinking, an
isolated world-state root, a one-request ceiling, and a 180-second real-time
cap. It ended normally after 180.0 seconds with zero model requests. The
600-frame CSV capture lasted 8.65 seconds:

| Track | Mean | p50 | p95 | p99 | Max | Frames >33.3 ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Frame | 14.42 ms | 13.37 ms | 21.99 ms | 40.85 ms | 48.52 ms | 9 / 600 |
| Game thread | 5.24 ms | 4.86 ms | 6.02 ms | 26.52 ms | 31.02 ms | 0 / 600 |
| Render thread | 14.24 ms | 13.39 ms | 16.47 ms | 40.88 ms | 48.54 ms | 9 / 600 |
| GPU | 9.76 ms | 9.54 ms | 11.78 ms | 12.28 ms | 12.85 ms | 0 / 600 |

Log: [`Codex_ViewProfile_20261007.log`](../../Saved/Logs/Codex_ViewProfile_20261007.log).
CSV: `Saved/Profiling/CSV/Profile(20261007_011646).csv`. This is one warm
spectator framing on an RTX 4080 Laptop GPU, not a user-controlled traversal,
PIE, packaged-game, or target-hardware guarantee. It does not resolve the
previously reported 8–13 FPS moving-PIE result. The screenshot run itself had
cold PSO-creation stalls and screenshot-encoding costs and is not used for
frame-rate claims.

## Repeat on the current built code (2026-10-07)

A second screenshot-free standalone sample used the current editor-target
binary, the same 1920×1080 spectator route, a 60-second warm-up, the existing
warm `Saved/LocalDDC`, an isolated world-state root, disabled agent thinking and
Python, a one-request ceiling, and a 180-second real-time cap. It ended normally
after 180.2 seconds with zero model requests. The 600-frame CSV is
[`Profile(20261007_065514).csv`](../../Saved/Profiling/CSV/Profile(20261007_065514).csv);
the runtime log is [`Codex_RuntimeProfile_20261007.log`](../../Saved/Logs/Codex_RuntimeProfile_20261007.log).

| Track | p50 | p95 | p99 | Max | Frames >33.3 ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| Frame time | 14.03 ms | 21.47 ms | 42.71 ms | 50.24 ms | 10 / 600 |
| Game thread | 5.04 ms | 6.58 ms | 22.65 ms | 40.53 ms | — |
| Render thread | 14.00 ms | 16.82 ms | 42.54 ms | 50.11 ms | — |
| GPU | 9.54 ms | 11.76 ms | 12.29 ms | 13.00 ms | — |
| RHI thread | 4.68 ms | 5.52 ms | 5.92 ms | 6.47 ms | — |

The current runtime placed 1,779,877 ground-cover instances, 359 Fab flowers,
14,727 spruce, 15,063 broadleaf shrubs, and 1,487 rhododendrons; the profiled
view submitted about 2.51 million GPU-scene instances.
`NumInstanceTransformUpdates` was 0 at p50/p95 and at most 1 in the sample. The
ten frames over 33.3 ms had roughly 13–29 ms render-thread time while GPU time
stayed around 11–12 ms. This points to a render/RHI-side tail in this standalone
route, but the counters do not identify one cause. It does not prove a 30-FPS
minimum: 10/600 frames exceeded that frame-time boundary, and p99 remained
42.71 ms.

This repeat confirms the earlier warmed standalone result on the current
binary, but still cannot reconcile it with the separate 8–13 FPS PIE report:
the modes, camera workload, and instrumentation are not matched. Avoid reducing
vegetation or changing species budgets on this evidence alone. The next useful
measurement is a paired, repeated PIE baseline and ground-cover-hidden profile
at the same camera, resolution, warm-up, and scalability settings, followed by
one structural ablation at a time. The editor was not available for that test.

## Fixed-camera foliage-sway diagnostic (2026-10-07)

To reduce viewpoint variation, three 1920×1080 standalone Game runs profiled the
same Tideglass camera from `Config/TideglassMotionProbe.json`. Each used the warm
`Saved/LocalDDC`, a fresh isolated world-state root, disabled agent thinking and
Python, a 60-second warm-up, a 600-frame CSV sample, a 180-second real-time cap,
and a one-request ceiling. All three ended normally with zero model requests.

| Condition | Frame p50 | p95 | p99 | Max | Frames >33.3 ms | Sway-update frames |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Default local sway (100 cm, 1 s) | 15.54 ms | 25.64 ms | 42.48 ms | 469.22 ms | 14 / 600 | 11 |
| Per-instance sway disabled | 15.96 ms | 20.01 ms | 32.05 ms | 474.27 ms | 6 / 600 | 0 |
| Local sway at 2 s cadence | 15.82 ms | 23.00 ms | 43.09 ms | 467.53 ms | 11 / 600 | 6 |

The no-sway sample improved p95 by 5.63 ms and brought p99 below the 33.3 ms
threshold, but still had six over-budget frames. The 2 s cadence was between the
other runs at p95 and did not improve p99. Most over-budget frames did not
coincide with the CSV's `NumInstanceTransformUpdates` counter: 2/14 in the
default run and 1/11 at 2 s. A single roughly 0.47 s frame-time outlier remained
in all three conditions while GPU maxima stayed below 20 ms. These are three
single standalone samples, so the outlier's cause and the sway effect need
repeated/PIE confirmation; do not globally disable resident- or wind-responsive
foliage from this evidence alone.

Logs: [`Codex_StabilityFixedBaseline_20261007.log`](../../Saved/Logs/Codex_StabilityFixedBaseline_20261007.log),
[`Codex_StabilityFixedNoSway_20261007.log`](../../Saved/Logs/Codex_StabilityFixedNoSway_20261007.log),
and [`Codex_StabilityFixedSway2s_20261007.log`](../../Saved/Logs/Codex_StabilityFixedSway2s_20261007.log).
CSVs: `Saved/Profiling/CSV/Profile(20261007_080758).csv`,
`Profile(20261007_081155).csv`, and `Profile(20261007_081533).csv`. The next
performance gate remains a paired, repeated PIE profile at the same camera and
scalability settings, because these standalone results do not explain the
reported 8–13 FPS PIE behavior.

## Current 11:00 gameplay-scale foliage gate (2026-10-07)

A single-view UE 5.8.3 editor SceneCapture reran `Tideglass` and
`TideglassGroundDetail` with transient ground cover enabled and world-state
loading disabled. The foliage preview generated 1,779,877 ground-cover
instances and ten nonblocking Tideglass cattails. The 50-interval gameplay-scale
Tideglass view reached 38.21 FPS wall-clock throughput, but its p95 frame
throughput was only 27.80 FPS, so the existing 30-FPS p95 gate failed. The
close-detail view reached 59.20 FPS wall-clock and 45.91 FPS p95 over 50 valid
intervals. This repeats the broad-versus-close mismatch on the current build;
it does not establish the separate reported PIE rate.

The editor SceneCapture frame shows dense, overlapping low plants around the
foreground pool, a long exposed brown slope, and narrow, evenly spaced distant
tree silhouettes. The close crop is almost entirely overlapping blades. The
pool and landmark proxies in these editor captures are blockout presentation,
not the runtime Game materials or transient rock landmark art, so these PNGs
are diagnostic composition references rather than highlight images. Screenshots:
[`02_Tideglass.png`](../../Saved/Viewpoints/2026-10-07_083121_h11.0/02_Tideglass.png)
and
[`02a_TideglassGroundDetail.png`](../../Saved/Viewpoints/2026-10-07_083121_h11.0/02a_TideglassGroundDetail.png).
Log: [`Codex_TideglassGroundCoverReview_20261007.log`](../../Saved/Logs/Codex_TideglassGroundCoverReview_20261007.log).

The result argues against increasing the global instance budget: the foreground
is already crowded while the middle/distant slope lacks mass, and the broad
view remains below the 30-FPS gate. The next pass should change spatial
composition and representation—more readable meadow/woodland groupings and
fewer overlapping near-camera blades—then compare the same view and frame-time
gate. Avoid changing global species or density budgets until the paired PIE
baseline and controlled visibility ablation identify which rendering cost is
dominant.

## Same-camera foliage-layer visibility ablation (2026-10-07)

The UE 5.8.3 `CaptiveSky2.Visual.Viewpoints` test was rerun at the same 11:00
`Tideglass` camera, 1600×900, with transient ground cover and world-state
loading disabled. Each run spawned the same population—1,780,640 ground-cover
instances, 14,727 spruce, 15,063 shrubs and 1,487 rhododendrons—then hid all
but the selected HISM layer before the capture. The test warmed ten frames;
each run produced 50 valid intervals, exceeding the 45-interval minimum, and
kept the 30-FPS p95 gate. Results:

| Visible layer | HISM components | Tideglass p95 FPS | Ground-detail p95 FPS |
|---|---:|---:|---:|
| Grass | 4 | 48.11 | 44.72 |
| Ground plants | 15 | 50.30 | 45.70 |
| Meadow (grass + ground plants) | 19 | 47.29 | 41.91 |
| Trees | 1 | 45.93 | 46.47 |
| Understory | 2 | 49.85 | 44.45 |
| Woodland (trees + understory) | 3 | 51.90 | 43.42 |
| All layers visible | 22 | 40.38 | 47.27 |

Every capture passed its automation gate. The previous full-population
27.80-FPS p95 result was not reproduced by the repeat, which reached 40.38 FPS.
These are single-run, editor SceneCapture measurements, not PIE gameplay or
packaged-build results; the run-to-run difference is evidence to repeat the
baseline, not proof of an engine or machine cause. The ablation does not identify
one isolated foliage family as the source of the reported 8–13 FPS PIE rate.
Logs: [`grass`](../../Saved/Logs/Codex_TideglassGrassOnly_20261007.log),
[`ground plants`](../../Saved/Logs/Codex_TideglassGroundPlantsOnly_20261007.log),
[`meadow`](../../Saved/Logs/Codex_TideglassMeadowOnly_20261007.log),
[`trees`](../../Saved/Logs/Codex_TideglassTreesOnly_20261007.log),
[`understory`](../../Saved/Logs/Codex_TideglassUnderstoryOnly_20261007.log),
[`woodland`](../../Saved/Logs/Codex_TideglassWoodlandOnly_20261007.log), and
[`all layers`](../../Saved/Logs/Codex_TideglassFullCoverRepeat_20261007.log).
The matched full-population frame is [here](../../Saved/Viewpoints/2026-10-07_115516_h11.0/02_Tideglass.png).

The evidence still supports improving the spatial composition before adding
instances, while keeping the paired PIE baseline open. More informative next
steps are repeated same-settings full-layer SceneCaptures and a matched PIE
profile after the editor is available; do not claim that the 30-FPS gameplay
target is met from these captures.

## Next foliage pass

Do not increase the global ground-cover budget based on these images. First
art-direct a Tideglass wet-edge/circulation composition: keep a clearly readable
open approach and pool outline while preserving the 10 cattails and distinct
water-edge plants. In parallel, strengthen the middle-distance woodland
silhouette on open slopes using bounded tree/grove distribution rather than
more tiny foreground clumps. Compare the same 11:00 frames and a warm,
screenshot-free 1080p profile before and after; retain the 30 FPS p95 target and
recheck resident paths and collision/navigation clearances. See also the
[worldbuilding priorities](../worldbuilding-ideas.md#2-replace-brute-force-foliage-density-with-ecological-compositions).
