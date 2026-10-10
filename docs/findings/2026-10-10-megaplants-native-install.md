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
