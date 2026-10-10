# Fab Megaplants native project install (2026-10-10)

The project now contains a native Unreal asset tree at
`Content/Megaplant_Library/` for Common Hazel, European Aspen, European Beech,
and Norway Maple. Read-only filesystem inventory found 296 `.uasset` files
(about 0.62 GB total). Each family has A-D `Tree_*` variants, per-tree skeleton
assets, material instances, textures, and PVE data/preset assets. This is a
substantial change from the 2026-10-07 inventory, which only found PN foliage
and Megascans surfaces. The installed files are under the project's ignored
`Content/` tree, so this is local installation state and is not included in
Git commits.

## Verification boundary

File presence and naming confirm that Fab delivered Unreal packages into the
project. They do **not** confirm package loadability, `StaticMesh` versus
`SkeletalMesh` class, successful material resolution, PVE plugin compatibility,
wind response, or runtime performance. No map or asset was modified, and none
of the new trees has been placed in the world.

## Candidate native PVE route

The UE 5.8.3 install contains the Procedural Vegetation Editor plugin binaries.
Its descriptor at
`Engine/Plugins/Experimental/ProceduralVegetationEditor/ProceduralVegetationEditor.uplugin`
describes a node-graph editor for Nanite-ready vegetation and loading
species presets; it marks the plugin experimental and disabled by default.
The descriptor declares dependencies on Dataflow, GeometryScripting, PCG, and
DynamicWind. The four Fab folders contain `PVE_*` data/preset packages, so PVE
is a plausible intended path for these species; the project does not list the
PVE plugin as enabled.

As an offline clue only, scanning the serialized strings of all 16 A-D
`Tree_*` packages found `ProceduralVegetation` and `SkeletalMesh` strings, but
no `StaticMesh` string. Package strings can come from references and do not
prove the exported object's class. This makes a direct HISM drop-in assumption
especially unsafe, while leaving the plugin-native preset route worth testing.
The next controlled step is to load one preset with PVE enabled in an isolated
editor/project, confirm the generated output type and its materials/wind, then
preview a small number of trees before changing the main project configuration.

`Scripts/Inspect-MegaplantAssets.py` is a read-only UE 5.8.3 audit for all A-D
tree variants and each family’s PVE assets. It reports loaded asset classes,
mesh bounds, LOD count, material slots, and whether each tree asset is a static
or skeletal mesh. It does not prove visual or runtime behavior. Run it only
when the existing editor is closed (or in a verified isolated scratch project)
to avoid competing for memory:

```powershell
& 'D:\Games\Epic\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' `
  (Resolve-Path '.\CaptiveSky_2.uproject') `
  '-ExecutePythonScript=D:/Projects - Athena/Unreal/CaptiveSky_2/Scripts/Inspect-MegaplantAssets.py' `
  -unattended -NoSplash -NoSound -NoZen -NullRHI `
  '-abslog=D:/Projects - Athena/Unreal/CaptiveSky_2/Saved/Logs/MegaplantAudit.log'
```

The project’s custom `Saved/Logs` path above contains spaces; if Unreal’s
command-line parser splits it, supply a short absolute log path with no spaces.
Do not treat an audit pass as approval to remove USD sources. Next, test one
well-bounded tree in a disposable preview scene: inspect shading, wind, LODs,
scale and GPU/CPU cost at the established Tideglass viewpoint. Only then decide
whether to add the native species to the Island scatter and retire any USD
workflow.

## UE 5.8.3 read-only audit result (2026-10-10)

The commandlet audit successfully loaded all 296 packages with PVE enabled only
for that invocation (`-EnablePlugins=ProceduralVegetationEditor`); the project
descriptor and user settings were not changed. Across the four families it
found 16 A-D tree variants, all `SkeletalMesh`, and **zero** `StaticMesh` tree
variants. Each family’s `ProceduralVegetation`,
`ProceduralVegetationGrowthDataAsset`, and
`ProceduralVegetationGrowerPreset` packages load with the plugin enabled. The
PVE graph itself has a protected `Graph` property, so Unreal Python cannot
inspect its nodes or export configuration; the read-only graph probe records
that limitation rather than modifying assets. Logs are in
`%TEMP%\CaptiveSkyAudit_20261010\Saved\Logs\MegaplantAudit_PVE_20261010.log`
and `MegaplantPVEAudit_Final_20261010.log`.

This confirms package compatibility/loadability, **not** that the current
skeletal trees are HISM-ready or that generated PVE output is present. PVE
remains experimental and disabled by default. No tree was generated or placed,
and no USD asset should be removed yet. The next gate is an editor-side,
single-species generation/export into a disposable preview folder, then visual,
material, wind, LOD/Nanite, and bounded runtime checks before any map/scatter
integration.
