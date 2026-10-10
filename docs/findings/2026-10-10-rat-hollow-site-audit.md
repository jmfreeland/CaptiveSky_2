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

## Next step

Find a genuinely sheltered patch within a navigable region, then place the
fallen-wood hollow there and verify its exact entrance orientation, landscape
contact, capsule/nav clearance, and gameplay-scale concealment before recording
the place or spawning Fenrus. Do not reuse the Raven's roost, the open fox
clearing, or guess a point inside the broad PCG volume. If a world-local check
is not available, add a scoped site-audit probe that reports trace hit
actor/component and queries the navigation system; do not save a provisional
placement. A small PVE/Megaplant preview is appropriate only after the editor
session has the PlantFactory mount and the selected pocket can be checked at
gameplay scale.
