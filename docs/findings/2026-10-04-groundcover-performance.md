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
**Fab project-asset check (2026-10-07):** a read-only inventory of the current project found 96 flower meshes, 72 ground-plant meshes, and 3 grass meshes in `PN_FoliageCollection/Meshes`. The only directory under the project's `Fab/Megascans` is `Surfaces`; no clearly named Megaplants pack or asset path was found in `Content`. This checks the local project only, not the user's Fab library, and a filename search cannot rule out renamed assets. Thus the user's library additions have not been confirmed as project assets or imported into this Island. When the editor/Fab library is available, inventory candidates there and import only a small set aimed at the observed middle-distance/woodland-silhouette gap; preview the fixed 11:00 composition and warm 1080p profile before increasing any scatter budget.

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

## Weather-driven Fab foliage wind bridge (2026-10-06, visual validation pending)

The project’s Fab foliage materials include `PN_WindAnimation` and use the shared
`PN_WindParameters` collection. The UE 5.8.3 `CaptiveSky2.Agent.IslandEnvironment` automation
loaded that real collection and confirmed its scalar inputs are named `WindDirection` and
`WindStrength` (defaults 0 and 1). Before this change, those Fab defaults drove built-in WPO
animation at fixed heading 0 and full strength. The weather-driven bridge now publishes
simulated ambient heading and normalized strength during normal play. Pass
`-IslandDisableFoliageMaterialWind` to restore the Fab fixed defaults, or supply a test
collection override to force publication in a fixture. CPU ground-cover sway remains a
separate behavior. The focused automation passed and checks default/opt-out selection and the
published values; this establishes parameter wiring, not that the parent material maps heading
correctly or looks natural.

The bounded UE 5.8.3 game preview eventually completed after a long cold shader/PSO startup.
It ran with CPU ground-cover sway disabled, agent thinking and Python disabled, one maximum
model request, isolated data/cache directories, and a 90-second realtime cap; it exited
normally at the cap. The paired baseline used the same settings and viewpoint without the
weather-wind flag and also exited normally. Both captured 200 CSV frames, all with frame
intervals under one second, from the 1600x900 Tideglass view on an RTX 4080 Laptop GPU.

| Capture | Frame-time median | Frame-time p95 | Render-thread p95 | GPU p95 | p95 throughput |
|---|---:|---:|---:|---:|---:|
| CPU sway off, fixed Fab defaults (`WindDirection=0`, `WindStrength=1`) | 14.89 ms | 20.31 ms | 19.22 ms | 17.49 ms | 49.23 FPS |
| CPU sway off, weather-driven Fab values | 14.84 ms | 18.06 ms | 18.50 ms | 16.95 ms | 55.38 FPS |

These single captures both clear the 30 FPS p95 screen for this one view, but are not a
packaged-game or broad-scene guarantee. This is a comparison of weather-driven MPC updates
against the Fab’s fixed default values—not wind enabled versus disabled. The small difference
is not evidence of a speedup; the useful result is that no obvious render-cost penalty from
the weather-driven updates appeared in this sample. Logs are
`Saved/Logs/Codex_FoliageMaterialWind_Game_20261006.log`
and `Saved/Logs/Codex_FoliageMaterialWind_Baseline_Game_20261006.log`; CSVs are
`Saved/Profiling/CSV/Profile(20261006_104426).csv` and
`Saved/Profiling/CSV/Profile(20261006_105631).csv`. The paired `002_Tideglass.png` captures
are in `Playtests/Codex_FoliageMaterialWind_20261006/Screenshots/` and
`Playtests/Codex_FoliageMaterialWind_Baseline_20261006/Screenshots/`. They show the same
composition, but static images (with the profiler counter visible)
do not establish that the foliage moves naturally or that the material’s heading convention
is correct.

Read-only UE 5.8.3 Python inspection loaded the Fab `PN_WindAnimation` function and enumerated
109 graph expressions. It exposes a scalar input named `Wind Gust Angle Rotation`, includes
sine/cosine expressions, and outputs `World Position Offset`. The separate MPC exposes scalar
`WindDirection` and `WindStrength` parameters. This supports a scalar-angle bridge but does not
prove how the parent foliage materials map `WindDirection` into that function input. The script
completed its graph enumeration, but the Python commandlet raised an access violation during
shutdown (`Saved/Logs/Codex_PNWindGraph3_20261006.log`); treat the emitted graph data as
diagnostic evidence, not a clean commandlet pass. No Content asset was changed.

The weather-driven bridge is now default-on, with `-IslandDisableFoliageMaterialWind` as a
reversible opt-out. The next visual check should inspect the parent PN material graph or capture
the same close foliage at a fixed camera over time with known headings, then verify direction
and amplitude. Preserve the existing bounded-session safeguards; repeat the A/B only if a
stable render-cost comparison is needed.

## CPU sway radius diagnostic (2026-10-06)

Two additional warmed, 120-second standalone Game runs used the same 1600x900
Tideglass view, 45-second CSV delay, 200-frame profile, isolated playtest data
roots, and a one-request cap (zero model requests were made). Both retained
1,779,877 ground-cover instances. The first used a 180 cm CPU sway radius; the
second disabled CPU ground-cover sway entirely while retaining material-driven
Fab wind. After excluding CSV section/header rows, their 200-frame profiles
measured:

| Diagnostic | Frame p50 / p95 | Game Thread p50 / p95 | Render Thread p50 / p95 | GPU p50 / p95 | Skinning p50 / p95 | Transform updates p50 / p95 |
|---|---:|---:|---:|---:|---:|---:|
| 180 cm CPU sway radius | 16.96 / 108.78 ms | 4.84 / 14.91 ms | 16.35 / 110.10 ms | 13.74 / 18.13 ms | 0.07 / 51.15 ms | 0 / 26 |
| CPU ground-cover sway disabled | 14.92 / 17.79 ms | 4.34 / 5.37 ms | 14.76 / 18.37 ms | 12.05 / 17.16 ms | 0.07 / 0.10 ms | 0 / 0 |

The disabled-sway capture clears the 33.3 ms p95 screen for this one view;
the 180 cm capture does not. This is not a matched causal A/B: they ran as
separate game processes with independently moving residents/animals and
different isolated world-state roots. In the 180 cm run, episodic skinning and
render-thread spikes coincide with the long frame tail, while the control does
not reproduce them. Do not attribute the difference to sway, and do not infer
that reducing the radius fixes the hitch. This also agrees with the earlier
delayed 180 cm profile, which missed 30 FPS p95 by a wide margin.

At that point, the attempted 180 cm production default was withdrawn and the
3000 cm default retained because the 200-frame captures were unpaired. The
later controlled cadence traces below supersede that provisional decision.

Evidence: `Saved/Logs/Codex_FoliageSwayRadius180_Warm_20261006.log`,
`Saved/Logs/Codex_FoliageSwayOff_Warm_20261006.log`,
`Saved/Profiling/CSV/Profile(20261006_194237).csv`, and
`Saved/Profiling/CSV/Profile(20261006_194656).csv`. Both runs used the existing
real-time/request safeguard settings and terminated normally.

## Controlled CPU-sway cadence and radius traces (2026-10-06)

The later captures used the same 1600x900 Tideglass motion-probe view, persistent
isolated data root (`Saved/Playtests/FoliageHitchTrace_20261006`), and shader/DDC
paths. Each trace began after a 45-second warmup and recorded 30 seconds. The
spectator had agent thinking disabled, a 120-second runtime cap and a one-request
maximum; all runs exited normally and made zero model requests. The world and
animals advanced between separate processes, so these are sequential same-view
comparisons, not frame-for-frame paired replays. The first broad-radius trace began
from a colder cache than the later trials; the high-cost tail nevertheless repeats
in the radius-180/0.1-second profile and is visible in the smaller-radius tests.

Finite positive game-thread frame events from the Insights exports give:

| CPU transform focus radius / update interval | Frames | Game p50 / p95 / p99 | Render p95 | Game frames >33.3 / >100 ms | 30 FPS p95 |
|---|---:|---:|---:|---:|---|
| Sway disabled; material wind retained | 1,977 | 14.7 / 18.6 / 22.9 ms | 18.6 ms | 5 / 1 | pass |
| 180 cm / 0.1 s (previous cadence) | 883 | 15.2 / 126.6 / 147.9 ms | 117.1 ms | 223 / 105 | fail |
| 180 cm / 1 s | 1,429 | 14.6 / 60.4 / 125.2 ms | 60.4 ms | 78 / 35 | fail |
| 180 cm / 2 s | 1,618 | 14.8 / 20.3 / 113.3 ms | 19.7 ms | 52 / 22 | pass, but delayed updates remain a concern |
| 100 cm / 0.1 s | 1,277 | 16.1 / 36.8 / 450.2 ms | 41.5 ms | 115 / 18 | fail |
| 100 cm / 0.5 s | 1,429 | 15.5 / 33.0 / 47.7 ms | 33.4 ms | 72 / 15 | marginal; render misses |
| 100 cm / 1 s (explicit override) | 1,492 | 15.4 / 19.9 / 90.5 ms | 19.9 ms | 42 / 15 | pass |
| 100 cm / 1 s (new defaults, no overrides) | 1,639 | 14.9 / 22.0 / 39.7 ms | 22.1 ms | 40 / 11 | pass |
| 100 cm / 1 s (no screenshot capture) | 1,387 | 15.6 / 21.5 / 452.5 ms | 21.9 ms | 44 / 17 | p95 pass; large synchronized outliers |

The repeated 180 cm traces show why radius alone was insufficient: reducing the
update rate from 10 Hz to 1 Hz cut game p95 from 126.6 to 60.4 ms, still failing
the gate. The 100 cm / 1 s setting kept game and render p95 below 23 ms in both
default captures, with and without screenshot capture. However, the no-screenshot
capture contained 17 frames over 100 ms (452.5 ms p99), versus 11 in the
screenshot capture (39.7 ms p99). Game, render, and GPU each recorded 17 >100 ms
events in the no-screenshot trace; all game events aligned with render events
within 20 ms and 15 of 17 aligned with GPU events. This tail is not isolated to
CPU foliage updates and may include a system-wide/runtime stall. The repeated
p95 result supports the cadence as a mitigation, but these outliers mean frame
pacing is not solved and the tail needs a cleaner controlled replay. By contrast,
100 cm / 0.5 s sits on the 33.3 ms boundary and misses on the render thread.

The Insights timer statistics also show the update-linked render tail shrinking:
at 180 cm / 0.1 s, `UpdateLumenScenePrimitives` reached 130.3 ms, `AddLumenPrimitives`
79.5 ms and skinning-view updates 92.5 ms. In the no-override 100 cm / 1 s repeat,
their maxima were 36.7, 22.5 and 20.3 ms respectively. `IslandWeather` itself
averaged under 0.3 ms in the passing repeat; the expensive work is downstream
scene-primitive/Lumen/skinning processing rather than its overall tick.

Accordingly, production defaults now use a 100 cm CPU-transform focus and a
1-second update interval. The command-line/CVar overrides remain available for
diagnostics, and the separate Fab material WPO wind remains continuous. Resident
and gust transforms still update locally; there is no claim that this cadence
removes occasional stutter or guarantees the same result in a packaged game or a
different gameplay composition. The existing ground-cover automation now sets
its own fixture radius and verifies interval accumulation/remainder behavior.
The UE 5.8.3 build and `CaptiveSky2.Agent.GroundCover` pass with the new defaults.

Trace logs: `Codex_FoliageHitchSway180Trace_20261006.log`,
`Codex_FoliageHitchSway180_1Hz_20261006.log`,
`Codex_FoliageHitchSway180_2Hz_20261006.log`,
`Codex_FoliageHitchSway100_0_1s_20261006.log`,
`Codex_FoliageHitchSway100_0_5s_20261006.log`,
`Codex_FoliageHitchSwayDefault100_1s_20261006.log`, and
`Codex_FoliageHitchSwayDefault100_1s_NoShots_20261006.log` under `Saved/Logs/`.
The raw `.utrace` captures are under `Saved/Profiling/Traces/`; timing CSV exports
were written to the local temporary directory. One trace artifact is named
`Sway180_2Hz`, but that run used a **2.0-second interval** (0.5 updates/second).
Screenshots are in the corresponding `Playtests/FoliageSway*/Screenshots/`
directories. They retain the same lush placement; static images do not verify
animation smoothness.
