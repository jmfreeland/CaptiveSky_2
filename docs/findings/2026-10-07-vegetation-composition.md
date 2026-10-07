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
