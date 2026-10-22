# Broader broadleaf coverage without more tree instances

## Change

`AIslandWeather::InitializeGroundCover` previously allowed a broadleaf grove to spend as many as 32 of its trees on one of the capped maple, aspen, or beech batches. The global quota of 600 per species was therefore consumed by a relatively small number of groves, leaving most of the Island dominated by spruce silhouettes.

The generator now limits broadleaf substitutions to five trees per grove. Remaining positions in each existing grove still use spruce. This keeps the total tree population and the per-species caps unchanged while spreading the three broadleaf silhouettes through more of the forest. A runtime log count reports how many accepted groves contain at least one broadleaf tree.

## Validation

- UE 5.8 Live Coding compile succeeded.
- After a clean reload, a request-disabled Simulate session regenerated 87,465 ground-cover instances and 14,430 total trees.
- The same run reported 600 Norway maple, 600 European aspen, and 600 European beech instances across 363 of 500 accepted groves; birch and hazel coppices remained at 39 and 57.
- No out-of-memory or texture-pool warnings appeared in the sampled D3D12/streaming logs.
- NVIDIA telemetry sampled 9,863 MiB of 12,282 MiB dedicated GPU memory in use. This remains high utilization; absence of a warning during this short run is not proof that longer play cannot exhaust memory.
- The 60-second session ended automatically at its cap with model requests set to zero. Safety defaults were restored to 1,800 seconds, 120 model requests, and continuous play disabled.

## Meadow-scatter follow-up

The first live daylight capture still showed a sharp lush/bare transition. The broad meadow pass used two hard cutoffs on its seeded habitat fields; those are now a soft 40–64% acceptance gradient, averaging 46% on a uniform field sample. This keeps the 96,000-candidate budget and 5% landscape-density setting unchanged while retaining a stronger tendency for lush communities.

- UE 5.8 Live Coding succeeded; `CaptiveSky2.Agent.IslandWeather` passed with no failures.
- A 30-second, zero-request Simulate regenerated 84,801 ground-cover instances from 8,317 valid terrain cells and 136,002 bounded traces. The prior baseline was 87,465 instances at the same configured budget, so this pass did not increase vegetation count.
- A fixed-angle live-world capture still shows broad bare-looking regions. The softened distribution is not yet a visual resolution; the next iteration should measure coverage by spatial cell and test a broader visitor-facing camera set before tuning density again.
- No out-of-video-memory message appeared during this short pass, but it did not capture a live GPU-memory telemetry sample and cannot establish safety for longer play.
- The Maple/Aspen/Beech missing-material-usage warnings remain in the editor log; foliage/Nanite appearance still needs validation after the in-flight material fix.
- The 30-second session ended automatically with zero requests. `MaxRealtimeSeconds`, `MaxModelRequests`, and `bContinuousPlay` were restored to 1,800, 120, and false.

## Fresh editor reload and water-collision check

After reloading `/Game/Maps/Island` in the editor, a new 30-second, zero-request Simulate pass rebuilt the runtime foliage. It again reported 84,801 ground-cover instances and 14,430 trees, with the 600/600/600 maple-aspen-beech mix spanning 363 of 500 groves, plus 39 birch and 57 hazel coppices. The Ocean subsystem logged that collision was disabled on the legacy `OceanPlane` and that the active plane uses `/Game/Materials/M_IslandOcean`.

This confirms the saved map loads and the runtime generators run after reload; it does not prove player movement transitions correctly between swimming and walking. No GPU-memory telemetry was captured in this pass. The editor still emitted missing material-usage warnings for the three Megaplant tree meshes, so the species count is not yet visual proof that their intended foliage materials render correctly.

For scale, the saved landscape AABB is 4.032 km by 4.032 km. The generator caps its center-sample grid at 40,000 cells, implying a roughly 20.2 m cell pitch for this bounds size; 8,317 cells passed the terrain/shore/grade filters. Treating each accepted sample as a whole cell gives a coarse valid-area proxy of 3.38 km². The 84,801 placements would then average about 25,000 instances/km², or one per 40 m² (about 6.3 m equivalent spacing) if they were uniform. This is only a budget sanity check: the actual land mask cuts through cells, the meadow total includes anchor patches, and placements are intentionally nonuniform. It does not explain the visible bare regions by itself.

The next useful distribution measure is a per-200 m tile report of eligible samples, actual placements, and zero/low-cover tile share, followed by a ground-level visitor capture. That can distinguish broad habitat shaping from an island-wide low footprint before either changing the density budget or the noise fields.

The generator now exposes default-off `CaptiveSky.Island.GroundCoverSpatialDiagnostics=1` instrumentation for that per-200 m report. It compiled with Live Coding, but was deliberately not enabled for a second regeneration after the latest preview exceeded the video-memory budget; the captured totals below therefore remain island-wide, not tile-level.

The 2026-10-22 editor reload/rebuild confirmed the current runtime mix again: 84,801 ground-cover instances and 14,430 trees, including 600 each of maple, aspen, and beech across 363/500 groves (plus 39 birch and 57 hazel coppices). One short PIE capture showed irregularly grouped ground cover and several tree silhouettes in the visitor view, so the “only spruce” observation is not true of this runtime; asset material correctness still needs checking after the separate Megaplant material fix. The viewport simultaneously displayed Unreal's red warning, “Video memory has been exhausted (179.7 MB over budget).” PIE was stopped after that single capture. This supersedes the earlier short-run note that saw no warning; do not repeat a long world playtest until VRAM headroom is improved.

The saved legacy `StaticMeshActor_0` uses `/Water/Meshes/S_WaterPlane_256`, is hidden, and has `NoCollision` / `NoCollision` profile. A world trace through offshore water at X=300,000 cm returned no hit, so that plane is not a walkable collision surface. This is not a possessed-player movement test, and does not by itself prove the swimming-to-shore transition. The current editor still reports `r.Nanite.Foliage=0` even though the saved project setting is true; the required editor restart has not happened, and the Nanite Assembly warning remains visible.

## Tideglass pollinator flowers (2026-10-22)

The fresh runtime log repeatedly showed `exclusion=8 ... placed=0` for eight reserved pollinator-flower anchors, despite the pool-edge ring being 7–8.5 m from Tideglass. The pool center is about 21.5 m from Wind Arch, 25.1 m from Listening Stones, and 34.0 m from the inn lantern. Their ordinary 26 m landmark exclusion therefore covered every pool-edge attempt, even though those blooms are a deliberate, collisionless exception.

Tideglass pollinator attempts now keep the existing pool-water clearance and use a 12 m buffer only for neighboring landmarks whose ordinary exclusion overlaps the reserved ring; distant landmarks retain the full 26 m clearance. The MeadowFlowerSpecies test checks that rule against the saved landmark layout across five deterministic seeds. UE 5.8 Live Coding succeeded and `CaptiveSky2.Agent.IslandWeather` passed (its fixture still emits the existing no-landscape scatter and world-context cleanup warnings).

A bounded 8-second, zero-request Simulate then regenerated 84,798 ground-cover instances and 14,430 trees. Tideglass flowers increased from 0/8 to 5/8 sites: 2 marsh-marigold and 3 yellow-flag iris instances, bringing total meadow flowers to 299. Two anchors still fail exclusion and one is blocked by an existing bloom, so all eight sites are not yet guaranteed. The editor viewport was at night and too distant to judge flower readability. GPU telemetry after stopping showed 7,130/12,282 MiB used (4,868 MiB free); no video-memory-exhaustion text appeared in the sampled editor log. This is a short verification only, not a sustained performance guarantee.

## Nanite Assembly support (2026-10-22)

The live editor displayed Unreal's warning that a Nanite Assembly had been encountered while Nanite Foliage support was disabled. Reading Project > Engine > Rendering confirmed `bNanite=true` but `bEnableNaniteFoliage=false` (`r.Nanite.Foliage=False` in `DefaultEngine.ini`). UE 5.8's [Nanite Foliage documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/nanite-foliage) calls the feature experimental, requires an editor restart after enabling it, and describes Nanite Assemblies as a way to reduce foliage streaming memory. Epic's examples are not a project-specific performance guarantee.

Enabled only `bEnableNaniteFoliage` through the Project Settings API; the saved config now has `r.Nanite.Foliage=True`. The editor remains open on the old setting, so no visual, frame-time, or memory result is claimed. Restart the editor, then verify the assembly warning clears and the three Megaplant species render with their intended materials. Compare VRAM and sustained frame time against the prior near-budget observation (`10.03/10.97 GiB`, one 21.3 ms HUD sample). The three separate missing material-usage warnings still need the in-flight parent/material fix; if the restart reveals worse stability or performance, revert this one project flag and reassess the imported assets.

## Limits and next checks

The runtime log proves broader spatial assignment, not that every tree's foliage material renders correctly or that the species read clearly from a visitor's view. The latest fixed-angle capture still shows patchiness; quantify per-cell cover next, then inspect additional visitor viewpoints. Measure steady-state frame time against the 30 FPS floor, repeat VRAM telemetry during a representative play session, and test movement across water with a possessed player before claiming those issues are resolved.
