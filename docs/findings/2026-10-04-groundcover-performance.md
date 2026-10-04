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

## Ground-cover blade scale A/B (2026-10-04)

The latest close-view image at the original 2.7x grass scale fills the entire frame with overlapping
blades, obscuring the ground and the silhouettes of nearby plants. In the same Tideglass detail
view with the meadow-and-tree layer subset, reducing the grass multiplier to 1.8x opened visible
ground between blades. The p95 throughput was 48.18 FPS at 1.8x (48/50 valid intervals) versus
46.26 FPS at 2.7x (50/50), so this is a readability adjustment, not an established performance
optimization. The existing placement count and spacing are unchanged.

The focused `CaptiveSky2.Agent.GroundCover` test passed with 1.8x in the isolated UE 5.8.3 target.
The broad, full-foliage Tideglass gate has not yet been rerun with this scale; do not infer that it
clears 30 FPS from the subset capture. Its trial source setting is now 1.8x. A subsequent scratch
build attempt waited on Unreal's build mutex while the main editor was open and was canceled before
compilation, leaving that full-view comparison pending.

Close-view captures: `2026-10-04_055700_h12.0/02a_TideglassGroundDetail.png` (1.8x) and
`2026-10-04_060126_h12.0/02a_TideglassGroundDetail.png` (2.7x), under
`Saved/CompileScratch/Codex_UnderstoryVerify_20261002/Project/Saved/Viewpoints/`.

## Spruce far-cull trial (2026-10-04, unresolved)

A clean UE 5.8.3 scratch UBT build succeeded (125 actions). The focused
`CaptiveSky2.Agent.GroundCover` automation passed against the scratch trial and verified the
250–400 m assertion. Its log is
`Saved/CompileScratch/Codex_UnderstoryVerify_20261002/Project/Saved/Logs/Codex_GroundCover_memoryddc.log`.

At noon with dense ground cover, the first trial capture reported p95 32.04 FPS at `02_Tideglass`
and 43.71 FPS at `02a_TideglassGroundDetail` (50 valid samples each; both above the 30 FPS screen).
The original 350–550 m baseline capture reported 48.61 and 51.97 FPS respectively. A further trial
run after the baseline, however, failed at `02_Tideglass` (2.78 FPS, only 43 valid intervals) while
the detail view measured 41.30 FPS. UE reported hundreds of first-use D3D12 PSO hitches in these
short captures, so this spread is not a reliable A/B performance comparison. The two saved
Tideglass overview images also look effectively the same and do not meaningfully expose the spruce
fade band; they cannot settle midground continuity.

The main tree is left on the original 350–550 m fade; the ecology assertion has been aligned to
that value. The incremental UE 5.8.3 scratch build and `CaptiveSky2.Agent.GroundCover` test passed
for this baseline. No trial code is retained. The evidence does not establish that the shorter
fade improves performance, and the current views are not spruce-visible enough to judge its
continuity. Revisit only with a stable spruce-visible viewpoint and a capture setup that avoids
first-use PSO contamination. The live editor and saved map were not edited.

## Listening Stones flowering edge (2026-10-04, awaiting visual validation)

The new worldbuilding direction calls for habitat composition around a Tideglass-to-Wind-Arch
view rather than simply adding more anonymous foliage. As a first localized change, the existing
512-site meadow-flower candidate budget now reserves 160 seeded sites for a 27.5–43 m annulus
around the tagged Listening Stones. This sits outside the 26 m landmark exclusion and inside the
45 m flower fade; existing 475 cm spacing, terrain/slope tests, species patching, and collisionless
HISM rendering remain in force. If Listening Stones is absent, all sites retain the previous broad
terrain-mask sampling. No foliage candidates or component instances were added to the fixed budget.

`CaptiveSky2.Agent.GroundCover` now asserts the ring's clearance/fade relationship and bounded
budget share. `git diff --check` passes. UE compilation and automation have **not** verified this
trial: both the main-project and isolated-scratch UBT invocations waited on the live editor's
compile mutex, so only those Codex-launched waiting processes were stopped. No screenshot or new
FPS result exists yet. Keep the trial provisional until a fresh UE 5.8.3 build/test and 11:00
Tideglass-view capture confirm the flowers land on the intended slope, read naturally, and preserve
the broad-view 30 FPS p95 target.

### Live-editor compile diagnostic (2026-10-04)

The responsive UE 5.8.3 editor process (PID 6344, started 2026-10-01) remains open. Unreal MCP
reports the saved `Editor > General > Live Coding` preference as enabled (`bEnabled=true`,
`startup=AutomaticButHidden`), but its `CompileLiveCoding` call returns “Live Coding is not enabled
for this session”; the current log has no `LogLiveCoding` entries. The ground-cover automation test
is registered in the loaded editor module, but it has not been run as evidence for the new source.
Saved preference state therefore does not establish that this already-running session initialized
Live Coding. No editor preference, map, or Content asset was changed. Wait for a user-approved
save/close/restart (or another safe UE 5.8.3 compile session), then build before running the test and
the 11:00 Tideglass capture; do not force-close this editor to release the build mutex.

### Fresh build and 11:00 validation (2026-10-04)

The previously open editor was absent on the next process check. The main UE 5.8.3
`CaptiveSky_2Editor Win64 Development` build completed successfully (nine actions, 36.74 s),
without waiting on the earlier mutex. The freshly linked project module and module manifest
were copied to the existing isolated test project. Its `CaptiveSky2.Agent.GroundCover` test
passed with exit code 0; this fixture verifies culling, navigation/collision exclusion,
bounded placement and wind behavior, but its constant assertions alone do not prove the
flowering annulus exists on the Island landscape.

The separate real-Island 11:00 capture supplies the placement evidence: the unchanged
512-site budget produced 354 Fab flowers, including 17 placed from the 160 reserved
Listening Stones sites, after 390 bounded terrain traces. The low reserved-site acceptance
shows that the local terrain/clearance filters reject most candidates. This is a placement
trial, not proof that the intended flower masses are visually legible.

`CaptiveSky2.Visual.Viewpoints` passed with exit code 0. At 1600x900, following ten warmup
frames, the broad `02_Tideglass` view measured 43.26 p95 FPS (51.77 wall-clock FPS) and the
close `02a_TideglassGroundDetail` view measured 42.39 p95 FPS (51.51 wall-clock FPS), both
with 50 valid / 50 raw intervals. This run clears the existing 30 FPS offscreen screen;
earlier variability means it is not a general gameplay-performance guarantee.

Visual inspection still shows a dense pale foreground, a largely bare middle distance,
thin tree trunks/silhouettes, placeholder-looking landmark materials and no convincing
wet-edge-to-meadow-to-woodland composition. The visual habitat milestone remains open.
Before expanding instance counts, improve plant scale/material readability and meadow/
woodland grouping, and consider broad-meadow fallback for rejected reserved flower sites.
The user's new Megaplants library additions have not yet been inventoried or imported.

Logs: `Saved/Logs/Codex_FlowerEdgeGroundCover_20261004.log` and
`Saved/Logs/Codex_FlowerEdgeTideglass_20261004.log`. Images:
`Saved/CompileScratch/Codex_UnderstoryVerify_20261002/Project/Saved/Viewpoints/2026-10-04_141225_h11.0/`.
The capture uses the current shared viewpoint source/configuration, mirrored into the
isolated project. It starts no gameplay session and uses `-ViewpointNoWorldState`; no
resident model turns were requested and no Island map or primary Content packages were saved.

## Broadleaf midground visibility (2026-10-04)

The 11:00 baseline exposes a sharp loss of fine cover beyond 45 m. A bounded trial keeps
fine grasses and the three older ground-plant forms at 25-45 m, while allowing the already
imported `ground_06_01` rosette (137 LOD0 triangles) and `ground_12_01` upright broadleaf
(362 LOD0 triangles) to remain through a 50-85 m fade/cull band. No placement density,
species selection, transform scale, candidate ceiling or instance count changes.

The main UE 5.8.3 build passed (seven actions, 15.56 s), and the isolated
`CaptiveSky2.Agent.GroundCover` automation passed. The latter checks the actual component
distances for both extended forms alongside collision/navigation exclusion, sway and clear
behavior. The real-Island scatter remains 1,779,413 ground-cover instances, 354 meadow
flowers, 14,727 trees, 15,063 shrubs and 1,487 woodland rhododendrons.

The first 1600x900 11:00 trial capture passed: broad-view p95 44.20 FPS across 49 valid / 50
raw intervals, close-view p95 56.78 across 50 / 50. Broad wall-clock throughput was only
17.50 FPS because of the one excluded interval; this cannot be presented as uninterrupted
gameplay performance. The same-hour baseline measured 43.26 broad / 42.39 close p95.
The images show additional low broadleaf cover between the near meadow and shrub layer,
but only a modest improvement. Pale patch colors, a still-bare farther slope and thin
woodland silhouettes remain unresolved. This does not complete the ecological-band target.

First trial log: `Saved/Logs/Codex_MidgroundTideglass_20261004.log`; images:
`Saved/CompileScratch/Codex_UnderstoryVerify_20261002/Project/Saved/Viewpoints/2026-10-04_142253_h11.0/`.
The repeat failed the broad-view gate: 19.30 p95 FPS with 50 valid / 50 raw intervals
(26.84 wall-clock FPS); the close view reached 52.05 p95 FPS with 50 / 50. Its startup
also logged timed-out connectivity probes and multi-second automation deltas. Because all
measured broad-view intervals were under one second, the failed p95 result cannot simply
be dismissed as an excluded connectivity stall. The same-range results are inconsistent,
so neither run proves a stable performance gain or a safe cost for the wider band.

The 50-85 m trial is rejected. Production ranges and the ecology assertions were restored
exactly to the committed baseline; no visibility-trial source change is retained. Repeat log:
`Saved/Logs/Codex_MidgroundTideglassRepeat_20261004.log`; repeat images:
`Saved/CompileScratch/Codex_UnderstoryVerify_20261002/Project/Saved/Viewpoints/2026-10-04_142541_h11.0/`.
Next, separate render cost from capture scheduling/first-use shader costs with a warmed
same-view GPU/render-thread profile. Then compose lower-cost habitat groups across the
middle distance rather than extending hundreds of thousands of broadleaf instances blindly.

Restoration verification: both production/test files match the committed baseline exactly
(`git diff --exit-code`), and the restored main UE 5.8.3 target built successfully (five
actions, 17.52 s). The fresh restored module was copied back into the isolated project.
Its same-view capture passed: broad p95 33.40 FPS with 49 valid / 50 raw intervals and close
p95 56.72 with 50 / 50. Broad wall-clock throughput was 11.75 FPS; startup/capture stalls
therefore still prevent a claim of stable interactive gameplay. The restored screenshot
is under `2026-10-04_143109_h11.0/` in the same scratch Viewpoints directory; the log is
`Saved/Logs/Codex_MidgroundBaselineRestore_20261004.log`. Only these findings are committed;
the rejected source trial is absent from the shipped tree and freshly built main module.
