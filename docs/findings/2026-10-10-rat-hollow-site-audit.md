# Fenrus hollow site audit (2026-10-10)

The user chose a **hidden hollow in fallen wood** as Fenrus' first home. The
mesh is available locally at
`/Game/Generated/ComfyBlender/RatHollow/SM_RatHollow_01`; the source cap was
removed and reimported into the live UE 5.8 editor. The editor reports 1,836
triangles, 256.4 x 135.7 x 134.1 cm bounds, and four box collision shapes. The
import report has no validation errors. In the mesh editor's Back view, the
entrance is open with a dark interior. This validates the asset import and
visible opening, not ground contact, navigation, or gameplay fit.

## Locations checked

- `Roost_East_FallenWood` at `(-99850, 101980, 2767)` is a real fallen-wood
  prop, but it belongs to the raven's roost cluster. Live viewport captures
  show the small saplings/undergrowth around it do not conceal the area; the
  surrounding meadow is open. Reject it as Fenrus' home: it would reuse the
  raven's site and does not meet the hidden-home brief.
- The woodland-fox candidate `(-100915, 97968, 2869)` was also viewed at ground
  level. The nearby slope is open grass facing the sea and is not a hidden
  hollow site.

Both checks used `EditorToolset.EditorAppToolset.CaptureViewport` on the live
`/Game/Maps/Island` editor world. `SceneTools.trace_world` returned ground or
prop hits around the roost, but no candidate has had navigation clearance
verified. The Island map is unchanged and no coordinate was written to
`Agents/Agent_Rat_01/places.json` (still empty).

## Procedural vegetation

`PluginToolset.PluginToolset.IsEnabled` confirms
`ProceduralVegetationEditor` is enabled in the running editor. The separate
`PlantFactoryPlugin` is enabled in the project descriptor but is not discovered
by this live session; it needs a project restart before its content can be
verified. The earlier [PVE availability probe](2026-10-10-pve-availability.md)
established that PVE and the PlantFactory content plugin load in an isolated
UE 5.8.3 scratch project, but did not validate interactive authoring or output
quality.

## PCG forest follow-up

The live editor contains one instance of `/Game/PCG/PCG_World_Forest`:
`PCG_ForestVolume_Island`. Its world bounds are approximately X/Y `-92,366`
to `-67,373` / `-92,369` to `-67,439` cm (about 25 km on each side). The graph
has 18 enabled nodes and 18 edges, with separate surface samplers for trees
(0.056 points/m²), grass (1 point/m²), and rocks (0.0204 points/m²). Its graph
parameter schema is empty, so this broad volume has no exposed density or
region controls. This establishes an existing PCG system to build on, not a
validated Rat site or proof that every visible plant came from this graph.

A provisional footprint-sized sample around `(-80000, -80000)` was rejected:
vertical traces spanning the hollow's roughly 2.6 x 1.4 m footprint returned
first-hit heights from 940 to about 1,277 cm. `SceneTools.trace_world` reports
only distance, not the hit actor/component, so the samples cannot distinguish
landscape, water, and vegetation collisions or prove ground slope. The live
MCP toolsets also expose no navigation-clearance query. No coordinate was
recorded and the Island map remains unchanged.

**Floating-forest follow-up (2026-10-11):** a bounded, read-only UE 5.8
commandlet reloaded `/Game/Maps/Island` and identified `PCG_ForestVolume_Island`
as `/Game/PCG/PCG_World_Forest.PCG_World_Forest`, with its PCG component
activated. The volume is centered at `(-80000, -80000, 500)` and spans
X `-92366..-67373`, Y `-92369..-67439`, Z `-1000..2418` cm. It is about 1.8 km
from the player start and outside saved Recast navigation coverage. The ocean
plane spans X/Y `±307200` cm and Z `812..1068` cm, so it covers the whole PCG
volume footprint and intersects its vertical bounds. Five diagnostic traces
across the volume hit the ocean at Z=940; when ignoring the ocean and volume,
the underlying landscape was only Z=68..677 at those samples. Graph inspection
found 16 nodes; `GetLandscapeData` connects to all three surface samplers
(`Trees`, `Grass`, `Rocks`). So the graph samples landscape, not the water plane.
At all five checked locations that landscape is submerged by 263–872 cm, which
can let tree crowns emerge through the water while their roots are below it—the
likely source of the reported floating appearance. The loaded, generated PCG
component owned 58,173 ISM instances at inspection: 54,918 seedlings, 3,249
trees, and 6 boulders. Transforming every tree mesh bound to world space shows
3,114 tree instances intersect the ocean plane and 135 are fully submerged;
none are entirely above it. This verifies the waterline intrusion, not just a
broad volume overlap. A rendered in-world view is still needed to judge how it
reads visually. As a reversible cleanup, the Island PCG component is now
`activated=false`, and all five generated ISM components have saved
`visible=false` and `NoCollision`; the graph and all 58,173 generated instances
are retained. A fresh commandlet reload verified those settings and unchanged
instance counts. This prevents the submerged hidden outputs from interfering
with player/resident movement or traces. Before reusing the graph, bind it to
above-water land, restore any desired collision profiles, and explicitly
reactivate/regenerate. The original pre-disable map backup is
`Saved/Backups/Island_PrePCGForestDisable_20261011_161538.umap` (SHA-256
`CCCC9742D686C0A4BFDF38ACF1F8438C836F66E340A395EC984D714EC767996A`). The
collision-cleanup restore point is
`Saved/Backups/Island_PrePCGForestCollisionProfileRetry_20261011.umap`; the
reusable guarded writer is `Scripts/Disable-IslandPCGForest.py`, and the
read-only audit is `Scripts/Inspect-IslandPCGForest.py`. The saved state was
verified after a clean reload in
`Saved/Logs/Codex_PCGForestCollisionVerifyFinal_20261011.log`.
The live Unreal MCP endpoint remains unavailable, and a rendered in-world view
is still outstanding.

## Saved-map survey and nav coverage

The read-only `Scripts/Inspect-IslandRatHollowCandidates.py` scan then loaded
the saved Island in the existing UE 5.8.3 scratch project. It inspected 180
actors and found zero explicit hollow tags. Its only two weak name/mesh hints
were `Roost_East_FallenWood` and `Roost_East_TrunkSupport`; both actually use
`/Engine/BasicShapes/Cylinder.Cylinder`, confirming they are blockout cylinders
in the Raven cluster rather than reusable natural log shelters. The scan did
not save the map. Log: [`RatHollowAudit run`](../../Saved/CompileScratch/RatAnimationProbe_20261010/Saved/Logs/CaptiveSky_2.log).

The live map has a `NavMeshBoundsVolume_Agent` spanning X `-105900..-95900`,
Y `95460..105460`, Z `1920..5920` cm, and a generated `RecastNavMesh-Default`
with bounds approximately X `-106704..-95836`, Y `94848..105716`, Z
`2220..3770` cm. The provisional PCG sample near `(-80000,-80000)` lies
outside that nav area. A known nearby woodland-edge fox pocket at
`(-100915,97968,2869)` is inside the nav area, but its bounded real-RHI view
shows the fox in tall grass in an open clearing; the nearest mature spruce is
about 8.15 m away. That is not a hidden hollow site either. Screenshot:
[`Fox woodland-edge frame`](../../Saved/Playtests/Codex_FoxCover_SparsePocket_Retry_20261010/Screenshots/001_Fox__Woodland_Edge.png).

## Bounded runtime search (2026-10-10)

The read-only `Island.RatHollowSiteAudit` probe was run in an isolated
NullRHI game against 12 overlapping circles distributed across the known
Recast bounds (centers X `-104000`, `-100000`, `-96000`; Y `97000`, `100000`,
`103000`, `105000`; radius 1,200 cm; 250 cm step). The 12 scans tested 828
footprints. All found landscape ground (`rejected ground=0`); 85 footprints
were rejected for excessive slope, and the remaining 743 failed the strict
spruce proximity/upper-crown filter. None qualified, and no capsule/nav
placement was attempted for those samples. The map and resident data remained
unchanged. Log: [`Run07`](../../Saved/CompileScratch/RatAnimationProbe_20261010/Saved/Logs/RatHollowSiteAudit_Run07.log).

This only establishes that the prototype's strict canopy criterion found no
site in these nav-bounds scans. It does not prove that no visually sheltered
site exists: foliage is non-colliding, the filter counts mature spruce crowns
rather than all vegetation, and most failures were grouped under the cover
counter. A new `IslandNavProbe.cpp` diagnostic split was attempted but its
scratch UBT process remained idle and was stopped; that unbuilt diagnostic
change was discarded. Keep the map untouched until candidate visibility and
rat-scale approach are verified.

## Globally ranked candidates reviewed (2026-10-11)

The read-only `Island.RatHollowSiteAudit` was repeated in a bounded NullRHI
session against the same 12-circle nav survey. It tested 828 footprints and
again found zero that met the strict mature-spruce crown rule. This pass also
ranked near-misses using understory proximity, nav footprint, and clear
approaches, so the cover rejection no longer hides useful alternatives.

The highest-scoring candidate is `(-104500, 105000, 2337.50)` cm: slope
9.2 cm, nearby understory 5/5, nav footprint 5/5, nav approaches 4/4, clear
approaches 4/4, nearest mature spruce 563 cm, score 275.2. A nearby candidate
at `(-104500, 105250, 2331.25)` scored 274.1 with the same understory/nav/
approach results. These are near the north edge of current nav bounds, so keep
the location in provisional status until a full hollow placement and route are
tested.

Two real-RHI Game screenshots were captured from ground level after shader
preparation, with agent thinking and Python disabled, an isolated data root,
zero model requests, and a 120-second realtime cap. See [`top candidate,
view 1`](../../Saved/CompileScratch/RatHollowSiteAudit_20261011_Run01/Screenshots03/001_1.png),
[`top candidate, view 2`](../../Saved/CompileScratch/RatHollowSiteAudit_20261011_Run01/Screenshots03/002_2.png),
and [`bounded run log`](../../Saved/CompileScratch/RatHollowSiteAudit_20261011_Run01/Logs/CandidateReview03.log).
Both show substantial fern/leaf/grass understory and a denser stand than the
earlier fox clearing, but also visible bare soil, widely spaced trunks, and no
fallen timber. That makes this a promising place to test a fallen-log shelter,
not a visually validated finished home. The first two candidates from the
southern search circle were also reviewed; they had good groundcover but were
weaker globally, so the northern pair replaced them for review.

The imported trial mesh was transiently spawned at the top candidate in an
isolated UE 5.8.3 editor-commandlet session, then destroyed without saving the
level. Its collision is enabled for Query-and-Physics, uses `BlockAll`, and
blocks Pawn. An actual Unreal Pawn-profile simple-collision capsule sweep using
the asset's nominal 16 x 24 cm Rat capsule passed from the west-facing entrance
into the hollow and back out. The center point and five-footprint ground audit
remain valid (9.2 cm height range). See the [`transient placement probe log`](../../Saved/CompileScratch/RatHollowSiteAudit_20261011_Run01/Logs/PlacementProbe06.log).

This proves the imported collision shell does not block that capsule sweep; it
does not prove CharacterMovement, a Rat-specific NavAgent, rebuilt NavMesh, or
visual concealment after placement. No Island map or resident place data was
changed. Do not write a home coordinate or spawn Fenrus until a real-RHI view
of the hollow in context and the remaining movement/nav checks pass.

## Disposable trial placement and runtime gate (2026-10-11)

The imported clearance-trial mesh was placed and saved into a local-only clone
of the Island at the top-ranked candidate, with its west entrance facing the
approach. The editor commandlet started directly on the clone and loaded it
once; it did not switch maps from Python. The placement log records bounds
center `(-104503.0, 104998.9, 2401.8)` cm, Query-and-Physics collision,
`BlockAll`, Pawn blocking, and `can_ever_affect_navigation=True`. The script
saved only `/Game/Maps/Island_RatHollowTrial` and exited cleanly. The main Island
and `Agents/Agent_Rat_01/places.json` remain untouched. See the
[`trial placement log`](../../Saved/CompileScratch/RatHollowSiteAudit_20261011_Run01/Logs/PlaceHollowInTrial.log).

A standalone real-RHI run against that clone was attempted with an isolated
data root, Python and agent thinking disabled, zero model requests, and a
120-second runtime cap. It did not reach engine initialization or load the
world within the 300-second startup guard, so the guard terminated only the
launched game process. The log records an UnrealBuildTool AutoSDK validation
exit code `-532462766` and an access-denied (`Error Code 5`) attempt to delete a
user-level `EditorSettings.ini`; it then stops before the world-ready marker.
There is no in-context screenshot and no evidence this is a content or
RatHollow failure. The game process produced no crash report. See the
[`bounded trial-game log`](../../Saved/CompileScratch/RatHollowSiteAudit_20261011_Run01/Logs/TrialGame.log).

`Scripts/Start-Spectator.ps1` now accepts a validated `/Game/...` map path and
uses that map's world-ready log marker, rather than hard-coding the main
Island. The script parsed successfully; its custom-map runtime path remains
unverified because UE startup stopped before map load. The trial clone is local
and ignored by Git.

## In-editor review and zero-request runtime gate (2026-10-11)

When Unreal MCP became available again, the active editor was on the main
Island and `/Game/Maps/Island` was clean. I opened the already-saved trial
clone in that editor and captured both trial viewpoints without saving or
changing the main map. In the editor viewport, the hollow reads as a pale,
uniformly cut log sitting alone on exposed brown ground. This is not yet a fair
habitat verdict: Island understory is scattered from `AIslandWeather::BeginPlay`
and is absent from the non-playing editor viewport. It does show that the prop
needs runtime context before we judge concealment, and its clean cylindrical
silhouette may need a more naturally broken/mossy art pass if it remains so
regular in gameplay.

To make that runtime check safe inside the live editor, the existing
process-start switch `-CaptiveSkyDisableAgentThinking` now has an equivalent
temporary console variable, `CaptiveSky.Agent.DisableThinking`. The
`AutonomousAgentAIController::Think` early-out uses either switch. Live Coding
compiled successfully; Unreal's CVar search reports the new variable and its
default value is `0`, so normal behavior is unchanged. I did not set it or
start PIE during this pass because the editor had other unsaved asset work
open. I restored `/Game/Maps/Island` and its original viewport camera; the main
map still reports clean. Runtime behavior of the CVar and the clone's actual
understory/concealment remain unverified.

## Then-upcoming step (superseded below)

This was the next step before the disposable-map foliage check documented
below. Its earlier statement that runtime understory had not been viewed is
historical; use the updated disposition and next steps at the end of this
document.

## Disposable-map foliage check (2026-10-11)

The live editor was switched from the clean main Island to the saved
`Island_RatHollowTrial` clone; no level was saved or edited. I verified
`CaptiveSky.Agent.DisableThinking=1`, ran Simulate with an eight-second
warmup, and then stopped PIE. `LogAgentSession` reported the normal safety
ceiling of 1,800 real seconds / 120 model requests and ended after **61.8
real seconds with 0 model requests and 0 in flight**. The planned 30-second
visual window was exceeded; this was still far below the hard play/request
limits. Thinking was restored to `0`, PIE was stopped, and the editor was
returned to `/Game/Maps/Island`; the main Island reports clean. `MHC_Aster`
remains dirty, so an editor restart is still not safe.

In the runtime foliage view, groundcover does obscure the opening from a
Rat-height approach, but large foreground leaves also make the entrance hard
to read. From a higher/wider angle the log remains in a broad bare-soil patch,
with the Inn and open coastline visible. This confirms runtime understory is
present but does not establish a satisfying hidden home. No Rat actor was
spawned; CharacterMovement, rat-scale navigation, a clear approach corridor,
and the final appearance are still unverified. The trial map, main Island,
`Agents/Agent_Rat_01/places.json`, and wildlife-home record were not changed;
keep Fenrus' home coordinate provisional.

### Updated next step

Do not promote this candidate yet. Improve or search for a more secluded
woodland site, then repeat a genuinely bounded real-RHI test with an explicit
30-second wall-clock stop: verify ground contact, preserve a readable
Rat-width route to the opening, inspect concealment from both Rat and human
views, and exercise Rat capsule/navigation movement. Only then persist the
home coordinate. Save `MHC_Aster` before restarting Unreal for the disabled
ProceduralVegetationEditor plugin; that plugin was not needed for this foliage
check.

**Entrance-facing art review (2026-10-11):** the separate, unimported
`RatHollow_ArtPass_20261011` FBX was re-imported and rendered with
`ASSET_PREVIEW_DIRECTION=-1,0,0`. The specimen is 259 × 140 × 133 cm, about
1,960 triangles, has five box-collision strips, four material slots, and the
pipeline reported no material/manifest problems. The open cavity and straw bed
are visible from the entrance, but the nearly circular, evenly framed opening
still reads like a cleanly cut tunnel rather than broken fallen wood; the
preview alone cannot judge site concealment or Unreal shading. The previous
side preview is preserved as `Preview-Side.png`; the imported trial and Island
were not changed. Before import, soften the cut-ring silhouette with an
asymmetric break or bark tears and check the moss/ground transition in a
woodland composition.

**Narrow broken-rim art pass (2026-10-11):** the source now builds the separate
`RatHollow_TornEntranceV2` export, leaving both earlier versions intact. It
removes two top-rim sectors, makes the entrance ring slightly less regular, and
adds one off-centre bark splinter; the rat passage and five collision boxes are
unchanged. The four source/clearance tests pass. Blender 5.2 rebuilt and
re-imported the FBX with no manifest/material problems: 262 × 140 × 133 cm,
about 1,994 triangles, four material slots, and five box-collision meshes. The
entrance render shows a small asymmetrical notch with moss visible behind it;
the side render still reads as a fallen trunk. This is a stronger local art
candidate, not Unreal/RHI or habitat-composition validation. It remains
unimported and unplaced; next compare it in the disposable woodland trial while
preserving the existing imported asset and do not assign Fenrus' home until the
site, approach, and navigation gates pass.
