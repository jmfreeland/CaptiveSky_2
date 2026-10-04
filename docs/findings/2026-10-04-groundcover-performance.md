# Tideglass ground-cover performance probe (2026-10-04)

The previous real-Island Tideglass capture enabled the full deterministic scatter: 1,779,413
ground-cover instances after 1,959,441 bounded placement traces. On the 1600x900 capture at
gameplay scale, the 50-frame sample averaged 0.57 FPS after a 10-frame warmup, below the
project's 30 FPS minimum. A close ground-detail capture of the same scatter averaged 48.29
FPS. The mismatch points to the far denser, broad view as the concern; the close-up result
does not excuse the gameplay-view failure.

The failed capture and its detail crop are in
`Saved/CompileScratch/Codex_UnderstoryVerify_20261002/Project/Saved/Viewpoints/2026-10-04_010000_h12.0/`.
The broad view is lush but visually dense, with a thin-looking horizon. The detail crop has
oversized/overlapping leaves. Those are separate art-direction issues from the frame-rate gate.

The first source experiment gave detailed grasses, low ground plants, and meadow flowers
80–140 m fade/cull ranges; cattails 90–150 m; shrubs and rhododendrons 240–400 m; and spruce
800–1200 m. In the corrected isolated UE 5.8.3 editor run, the bounded
`CaptiveSky2.Agent.GroundCover` automation test passed, and the full Island preview placed
1,779,413 landscape instances after 1,959,441 traces. However, the `02_Tideglass` RHI capture
measured only 3.28 FPS (50 measured frames after 10 warmup), failing the 30 FPS gate. The
nearby `02a_TideglassGroundDetail` capture reached 39.02 FPS. The broad composition therefore
still fails despite initial distance culling; these numbers are not a performance fix.

The tighter 25–45 m ground-detail, 80–140 m understory, and 350–550 m spruce experiment
improved `02_Tideglass` to 18.44 FPS and `02a_TideglassGroundDetail` to 45.92 FPS, but still
failed the broad-view gate. The screenshot shows a bare middle distance after detail culling.
The next experiment used 10–25 m fine grass/plants/flowers, 30–45 m cattails, 40–100 m
shrubs/rhododendrons, and 350–550 m spruce. It measured 15.30 FPS broad and 49.13 FPS close,
still below the gate. Its screenshot (`2026-10-04_023914_h12.0/02_Tideglass.png`) confirms that
the tight ground-detail range hollows the middle distance while leaving thin, regularly spaced
tree silhouettes; this balance is rejected. The capture log also contains long pre-test and
in-test frame deltas and timed-out connectivity checks, so the inconsistent broad-view numbers
are not yet a reliable basis for choosing a final range. Do not accept any cull setting until
the broad view reaches 30 FPS and the midground reads naturally, with repeated clean captures.
The corrected scratch launch loads the project module/map and runs tests. The user's open
editor was not touched.

The project already contains the Fab temperate foliage collection at
`Content/PN_FoliageCollection` (the ground cover uses its grass, plant, and now six flower
meshes), plus `Content/PN_interactiveSpruceForest`. Three newly enabled flower forms were
previewed in the editor; the bounded candidate-site count and total flower instance budget
are unchanged. No new marketplace content was imported because the needed free variety was
already present. This keeps the next iteration focused on proving the performance budget
before adding more visible instances.

At this stage the cause of the full-view slowdown was still unisolated. The earlier 10–25 m
ground-detail range is visually rejected; seek a natural, varied midground as well as the 30 FPS
floor.

## Frame-time gate correction (2026-10-04)

The initial throughput calculation divided 50 captures by total wall time, so unrelated long
automation-thread stalls counted as rendered frames. The isolated capture log confirms that
`UnrealEditor-Cmd` repeatedly blocks on `https://www.google.com/generate_204`; the automation
controller explicitly logs these as very large deltas. The capture now reports raw wall-clock
throughput and separately gates the 95th-percentile interval from individual capture updates,
excluding only intervals over one second and requiring at least 45 of 50 usable samples.

After rebuilding and rerunning the same 1600x900 two-view test, the broad Tideglass view reported
0.38 wall-clock FPS and 2.11 p95 FPS across 43 valid / 50 raw intervals. The close detail view
reported 44.29 wall-clock and 37.86 p95 FPS across 50 / 50 intervals. The broad view remains a
real failure even after excluding the largest connectivity stalls; the close view clears the
30 FPS p95 gate. This run does not establish packaged-game performance, but it does establish
that the broad scene needs substantial render-cost work. Build succeeded; the viewpoint test
failed as intended on the broad-view gate. Log: `Codex_TideglassFrameP95_20261004.log`.

The next experiment profiled broad-view render cost by layer (ground detail, understory, trees,
landscape), as recorded below. The earlier tight 10–25 m range remains visually rejected; do
not trade away natural midground cover for an unproven FPS gain.

## Single-layer render diagnostics (2026-10-04)

Added `-ViewpointGroundCoverSoloLayer` (and `-GroundCoverSoloLayer` to the capture script) to
hide all but one transient HISM group for an offscreen capture. The landscape-only view measured
24.55 p95 FPS broad / 45.79 close. With individual layers visible, broad p95 was: trees 44.46,
understory 42.71, grass 39.36, ground plants/flowers/cattails 37.50 FPS. The combined meadow
(grass plus ground plants) reached 38.31 FPS. These captures used 50/50 uncontended intervals
and passed the 30 FPS gate except for the foliage-free landscape baseline.

The full foliage configuration previously measured 2.11 p95 FPS with only 43/50 valid intervals.
Follow-up tests found meadow+trees at 39.20 and meadow+understory at 34.00 p95 FPS. The full
configuration still needs a substantial total-cost reduction; no single group or two-group
combination explains the slowdown. The baseline landscape itself is marginally under 30 FPS,
so the quality target for a sustainable combined view should remain >=30 p95 without visually
clearing the midground.

## Full-scene recapture and additional flowers (2026-10-04)

The current intermediate ranges (25–45 m fine ground cover, 35–55 m cattails, 80–140 m
understory, and 350–550 m spruce) passed the ground-cover cull-range assertions. With eight
flower meshes enabled, the latest full 1600x900 Tideglass capture still missed the performance
gate: broad view 25.48 p95 FPS over 48 valid / 50 raw intervals; close detail 46.33 p95 FPS over
50 / 50. The scene generated 1,779,413 landscape cover instances and 15,063 broadleaf shrubs.
A same-range full-scene run immediately before the flower update measured only 3.76 broad p95,
so total render cost is evidently variable in this offscreen editor environment. The latest
screenshot still shows a dense foreground, bare middle distance, and thin tree silhouettes:
`Saved/CompileScratch/Codex_UnderstoryVerify_20261002/Project/Saved/Viewpoints/2026-10-04_035841_h12.0/02_Tideglass.png`.
The 30 FPS broad-view gate is not met; do not treat the intermediate cull ranges as final.

A trial cap of 1.2 million broad-meadow candidates placed 1,114,356 instances (versus
1,779,413 at the 1.92 million cap) but scored only 8.28 broad p95 FPS and 43.09 close p95 FPS.
The image looked broadly similar with slightly lighter near-field overlap. Since it did not
demonstrate a performance gain and weakens the foliage target, that cap was rejected and the
source budget restored to 1.92 million. The inconsistent captures need a more reliable
render-time profiler or controlled editor conditions before density is cut again.

To add flower variety without increasing the fixed 512-site / 489-instance accent budget, two
more meshes already present in the ignored Fab foliage pack were enabled: `flower_02_01` (purple
spike) and `flower_03_01` (small pink sprig). The eight-species flower list and asset references
pass the isolated UE 5.8.3 `CaptiveSky2.Agent.GroundCover` test. No new external Fab package was
downloaded: the free Cinematic Dragonfly listing is a promising animated-fauna candidate, but
the Fab library action did not respond in this browser session. It is UE 5.4 content supplied
as a complete project, so it will need a Fab account/library claim and a project migration step
before integration.

The next performance experiment should lower global meadow instance density or add a simpler
far-distance representation, then recapture broad and close views. The next fauna asset step is
to claim and migrate the Cinematic Dragonfly only once Fab library access is available.

## Close-view layer interaction probe (2026-10-04)

To isolate the overloaded Tideglass ground-detail view, the same 1600x900 camera and 50-frame
real-RHI capture were rerun with selected HISM groups visible. The foliage-free landscape and
individual or paired layers behaved differently:

| Visible layer(s) | p95 throughput | Valid intervals | Result |
|---|---:|---:|---|
| grass | 46.84 FPS | 50/50 | pass |
| ground plants, flowers, and wetland plants | 42.50 FPS | 50/50 | pass |
| spruce | 48.51 FPS | 50/50 | pass |
| meadow (grass + ground plants) | 32.54 FPS | 50/50 | pass, little headroom |
| meadow + shrubs/rhododendrons | 38.36 FPS | 48/50 | pass |
| trees + shrubs/rhododendrons | 45.73 FPS | 50/50 | pass |
| meadow + spruce | 10.37 FPS | 49/50 | fail |
| full ground-cover scene | 6.02 FPS | 46/50 | fail |

These are automated offscreen scene-capture intervals, not a packaged-game or target-hardware
benchmark. The spruce-plus-meadow result is nevertheless a strong diagnostic lead: neither group
alone, nor meadow plus understory, explains the slowdown. The severe regression appears when the
meadow and spruce layers render together. The precise GPU/CPU mechanism remains unprofiled; treat
screen-space overlap, foliage material cost, and HISM cluster/instance cost as hypotheses, not
proven causes. Avoid thinning one species or importing more high-detail vegetation until a
targeted adjustment improves both this close composition and the broad view without hollowing
the midground.

Logs: `Codex_TideglassGrassOnly_20261004.log`, `Codex_TideglassGroundPlantsOnly_20261004.log`,
`Codex_TideglassTreesOnly_20261004.log`, plus the earlier `Codex_TideglassMeadowOnly_20261004.log`,
`Codex_TideglassMeadowUnderstory_20261004.log`, `Codex_TideglassWoodlandOnly_20261004.log`,
`Codex_TideglassMeadowTrees_20261004.log`, and `Codex_TideglassSpecies_20261004.log`.
