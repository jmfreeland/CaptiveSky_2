# Megaplant PVE availability audit (2026-10-19)

## Result

UE 5.8.3 mounts the enabled `ProceduralVegetationEditor` engine plugin. A
read-only Python audit loaded the installed Megaplant PVE graphs, growth-data
assets, and grower presets for Common Hazel, European Aspen, European Beech,
and Norway Maple Forest. The library contained 296 assets; the audit found 12
PVE-related graph/data/preset assets across those four species.

Each graph loads as `ProceduralVegetation`, but its `Graph` property is
protected from Unreal Python. The existing Python interface can inventory and
load these assets; it cannot inspect or export their internal graph. This
confirms the earlier finding that a complete Static Mesh candidate must be
exported through the PVE editor UI, not by a Python-only script.

## Method and boundary

The audit ran from the isolated `Codex_DragonflyFrogResponse_20261019` scratch
project. Its `Content/Megaplant_Library` junction points to the installed
library, and the run used `-NullRHI`, `-DDC-ForceMemoryCache`,
`-CaptiveSkyDisableAgentThinking`, a 120-second play cap, and a zero-model-
request cap. `Scripts/Inspect-MegaplantPVEGraphs.py` only lists and loads
assets; it does not save them, invoke the exporter, modify the Island, or
change project settings. The log reports `assets=296 graph_assets=12` and
completed successfully:
`Saved/CompileScratch/Codex_DragonflyFrogResponse_20261019/Project/Saved/MegaplantPVEAudit_20261019.log`.

This is availability/API evidence only. It does not prove that an exported
tree has a useful silhouette, correct materials, wind, a performant batch, or
good Island composition. The current desktop window inventory did not expose
Unreal, so no editor UI export was attempted. Do not place or scatter a
Megaplant on the Island on the strength of this audit alone.

## Next step

When the editor UI is available, export one complete PVE candidate (Norway
Maple Forest is a reasonable first woodland-transition probe) into a disposable
test folder. Preview it under the fixed Tideglass-to-Woodland lighting, inspect
its silhouette/material/wind appearance, then measure a small batch before
considering any map placement. Keep the existing Beech's rejected dark/sparse
comparison as a warning against assuming that PVE availability guarantees a
usable canopy.
