# Floating PCG forest: current Island editor audit — 2026-10-22

## Result

The currently loaded editor level is `/Game/Maps/Island` and PIE is not running.
The read-only Unreal scene inventory found only the default `PCGWorldActor_0`
when searching actor names for `PCG`; that actor is at the origin and exposes
no components. The PCG toolset reported no graph instances in the loaded scene.
Type-filtered actor searches also found no `PCGVolume`, `PCGPartitionActor`,
`PCGIsolatedActor`, `PCGBuilderVolume`, `InstancedFoliageActor`,
`ProceduralFoliageVolume`, or project `IslandInstancedSkinnedFoliage` actors.
The editor viewport showed no floating PCG forest.

This is evidence that the reported old PCG forest is not active/present in the
current loaded Island session. It does not establish the state of the separate
`/Game/Maps/PCG_World` test map or delete/alter its preserved forest graph.
Those assets remain untouched. If the forest appears again, first confirm the
active map and whether the observation is in PIE; do not delete the test graph
based only on a viewport report.

## Separate vegetation diagnosis

The current non-PIE viewport still reads as a broad exposed slope with isolated
ground clumps. `IslandWeather`'s editor HISM components (`ShoreGrassA`,
`ShoreGroundPlants`, and `IslandSpruce`) report `bVisible=false`, so this
editor-only view is not a valid preview of their runtime scatter. The previous
bounded PIE log confirms that runtime population is created, but not that it
looks natural at player scale. Keep the active habitat-gradient and
material/performance work as the next review gates; do not infer a density fix
from the PCG actor audit.
