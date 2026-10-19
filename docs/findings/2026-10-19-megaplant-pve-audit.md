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

## Mesh and wind follow-up (2026-10-19)

The existing read-only `Scripts/Inspect-MegaplantAssets.py` inventory
enumerated 296 library assets and loaded 16 A-D tree variants: four each for
Common Hazel, European Aspen, European Beech, and Norway Maple. All 16 are
`SkeletalMesh`; the inventory found zero StaticMesh tree variants. Their
classes and bounds are asset metadata only, not proof of useful appearance.
Log: `Saved/CompileScratch/Codex_DragonflyFrogResponse_20261019/Project/Saved/MegaplantMeshAudit_20261019.log`.

The isolated UE 5.8.3 target was up to date, and
`CaptiveSky2.Agent.InstancedSkinnedFoliageProvider` passed under NullRHI with
the Megaplant library mounted, `-DDC-ForceMemoryCache`, agent thinking
disabled, and the model-request cap set to zero. Its installed-mesh checks
load the four representative trees, require enabled `UDynamicWindSkeletalData`,
and verify the foliage host selects and clears the transient
`UDynamicWindData` transform provider. Log:
`Saved/CompileScratch/Codex_DragonflyFrogResponse_20261019/Project/Saved/InstancedSkinnedMegaplantProvider_20261019.log`.

This verifies a viable UE 5.8 wind-provider path for the skeletal trees, not
rendered wind, material quality, a batch frame-time budget, or Island
composition. The separate Beech visual rejection still applies. In the latest
UI check the editor reached its Home Panel, but a blank overlay obscured it, so
no PVE export UI was used and no project was opened.

## Next step

When the editor UI is available, export one complete PVE candidate (Norway
Maple Forest is a reasonable first woodland-transition probe) into a disposable
test folder. Preview it under the fixed Tideglass-to-Woodland lighting, inspect
its silhouette/material/wind appearance, then measure a small batch before
considering any map placement. Keep the existing Beech's rejected dark/sparse
comparison as a warning against assuming that PVE availability guarantees a
usable canopy.

## Live editor visibility recheck (2026-10-19)

A read-only desktop capture confirms the Unreal Editor window is present with
the Island loaded, `TideglassPool` selected, and 180 actors in the Outliner. The
editor's Output Log still shows Live Coding reporting “No object changes
detected,” so it does not prove the latest working-tree module is loaded. A
modal asks whether to reopen an asset editor from the previous session; it was
left untouched. No PVE graph was exported, no asset was edited, and the map was
not saved. The Unreal MCP tool catalog did not return after 90 seconds and its
pending call was stopped; the separate desktop capture worked. Next, after the
modal is cleared, use the PVE editor UI for one disposable export and preview
under Island lighting. Do not interpret editor visibility as visual approval
or as evidence that Megaplant foliage is ready for Island placement.

## UE 5.8 PVE export capability (2026-10-19)

Epic's [UE 5.8 PVE documentation](https://dev.epicgames.com/documentation/unreal-engine/procedural-vegetation-editor-pve-in-unreal-engine?lang=en-US)
confirms that its Export utility bakes graph output into renderable geometry
and can produce either a standard Static Mesh or a Skeletal Mesh. The
[export-parameter API](https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/ProceduralVegetation/FPVExportParams)
clarifies the relevant trade-off: a skeletal export carries bones and can use
the Dynamic Wind simulation preset; a static export has no wind animation.
This makes the existing Megaplant PVE graphs a direct path to testable,
wind-enabled woodland trees, not merely an inventory source. The local Python
audit still cannot export because graph execution/export is editor/plugin work;
the next candidate remains Norway Maple Forest, exported into a disposable
folder as Skeletal Mesh with a wind preset, then checked for rendered canopy,
materials, and bounded instance cost before any Island placement.
