# Bounded tree-spatial and ocean-collision audit — 2026-10-31

## Run

The current worktree source was built against a short-lived scratch project at
`Saved/CS58/`, whose `Content` directory is a junction to the main D: project.
The live editor and Island package were not changed. The hidden standalone Game
run used UE 5.8, `-NullRHI`, the startup tree-spatial diagnostic,
`-CaptiveSkyDisableAgentThinking`, an isolated `DataRoot`, a 120-second real-time
cap, and a zero-model-request cap. Unreal reported a dummy Dynamic RHI, confirming
that this run did not use the D3D renderer. The Island reached world-ready in 33
seconds, then ended normally after 120.2 seconds with zero model requests.

The generated report is
[`TreeSpatialNullRHI_20261031.md`](../../Saved/CS58/Saved/SessionReports/TreeSpatialNullRHI_20261031.md);
the full Unreal log is
[`TreeSpatialNullRHI_20261031.log`](../../Saved/CS58/Saved/Logs/TreeSpatialNullRHI_20261031.log).

## Evidence

- Mature trees occupy 117 200 m tiles; 111 tiles (95%) contain at least two
  species. The richness breakdown is 6 single-species, 10 two-species, 21
  three-species, 35 four-species, and 45 five-species tiles.
- The mature-tree totals are 2,014 spruce and 900 each of Norway maple,
  European aspen, European beech, and black alder. Six sparse edge tiles are
  examples of single-species coverage; this is not a sign that any broadleaf
  population is missing from the island.
- Game startup logged `IslandOcean: disabled collision on legacy OceanPlane`
  and applied `/Game/Materials/M_IslandOcean`. This confirms the runtime code
  finds the saved Island actor and removes its collision. It does not prove the
  player capsule can no longer walk on water in the real-RHI, player-controlled
  game; a direct shoreline movement check remains open.
- The run had no render, FPS, VRAM, or gameplay-scale silhouette evidence.

## Decision and next step

Do not raise the global tree count on the basis of the editor's sparse hillside
view. The island-wide distribution already has broad five-species coverage.
Next, make a matched real-RHI, player-height pass through the open slope and
woodland edge, with Nanite Foliage active, and judge the silhouettes and cover
readability while measuring frame time and whole-device memory. If the local
view still reads as one species, adjust placement or distance readability from
that evidence rather than increasing the total budget. In the same bounded Game
pass, verify grounded movement across the shoreline and confirm the player
enters water or falls instead of walking on the legacy plane.
