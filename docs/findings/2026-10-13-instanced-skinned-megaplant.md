# UE 5.8 instanced-skinned Megaplant probe

## Question

Can the installed European Beech skeletal outputs add a useful mid-distance
canopy at the woodland transition through UE 5.8's
`UInstancedSkinnedMeshComponent`?

## Probe

`Scripts/Preview-InstancedSkinnedMegaplant.py` loads the saved Island as a scale
reference, spawns one transient instanced-skinned foliage actor and one ordinary
`SkeletalMeshActor`, then captures each component separately after the renderer
has ticked. The script does not save the map or any source asset. The native
`AIslandInstancedSkinnedFoliage` wrapper is collisionless and accepts a skeletal
mesh and local instance transforms; animation index 0 is used, but no transform
provider is assigned.

## Results

- UE 5.8.3 accepted the Beech skeletal mesh in both rendering paths and retained
  one `InstancedSkinnedMeshComponent` instance. The component exposes two
  material slots: `MI_European_Beech_Bark_01` and
  `MI_European_Beech_Foliage_01`. UE logged that the bark material needed the
  `InstancedSkinnedMesh` usage flag; no source material was saved or edited.
- With the project's current Nanite foliage setting off, Beech A (23.82 m
  bounds height) and B (18.48 m) render as sparse, bare-looking branches. The
  ordinary skeletal-mesh capture of A is similarly bare, so the omission is
  not unique to the instanced component. Captures:
  [A, instanced](../../Saved/Playtests/Codex_InstancedSkinnedBeech_20261013_Compare/InstancedSkinned_BeechA.png),
  [A, ordinary skeletal mesh](../../Saved/Playtests/Codex_InstancedSkinnedBeech_20261013_Compare/SkeletalMesh_BeechA.png),
  [B, instanced](../../Saved/Playtests/Codex_InstancedSkinnedBeech_20261013_VariantB/InstancedSkinned_BeechA.png).
- A second disposable launch supplied startup ini overrides for
  `r.Nanite.Foliage=1`, `r.Nanite.AllowVoxels=1`, and
  `r.Nanite.AllowAssemblies=1`. UE logged all three as 1 and built the leaf
  assemblies without the earlier disabled-feature warnings. The leaves then
  appeared in both render paths, but only as coarse, very dark clusters in the
  isolated capture; the result is not yet acceptable for Island placement.
  The ordinary and instanced versions are visually similar, so Nanite foliage
  support—not batching alone—is the missing asset-rendering condition. Captures:
  [A, instanced with Nanite foliage](../../Saved/Playtests/Codex_InstancedSkinnedBeech_20261013_NaniteConfigOverride/InstancedSkinned_BeechA.png),
  [A, ordinary skeletal mesh with Nanite foliage](../../Saved/Playtests/Codex_InstancedSkinnedBeech_20261013_NaniteConfigOverride/SkeletalMesh_BeechA.png).
- An attempted `-ExecCmds` toggle was rejected because these renderer CVars are
  read-only after startup. The successful `-ini` overrides were process-only;
  `Config/DefaultEngine.ini` remained unchanged. Do not enable this experimental
  renderer setting project-wide based on this isolated result.
- The foliage material audit found that `MI_European_Beech_Foliage_01` uses the
  plugin's `MA_Foliage_Trees` parent and has a blue-gray `BaseColor Tint`
  (`[0.3214, 0.3684, 0.5, 1]`), plus the expected foliage color/normal textures
  and translucency/season controls. A second process-only capture applied a
  green `Base Color Tint` (`[0.48, 0.65, 0.35, 1]`) to transient dynamic
  material instances on both render paths. This makes the cards greener, but
  the tree remains visibly sparse and blocky; color alone is not the fix. The
  material instances, map, and project settings were not saved. Captures:
  [green-tinted instanced](../../Saved/Playtests/Codex_InstancedSkinnedBeech_20261013_GreenTint/InstancedSkinned_BeechA.png),
  [green-tinted ordinary skeletal mesh](../../Saved/Playtests/Codex_InstancedSkinnedBeech_20261013_GreenTint/SkeletalMesh_BeechA.png).
- The component reports no transform provider. Wind and animated per-instance
  bone transforms therefore remain untested. These one-tree scene captures do
  not measure runtime cost, LOD behavior, or a multi-instance batch.
- The UE 5.8.3 PVE exporter has no exposed Python/UFUNCTION entry point in the
  installed plugin headers: `FPVExporter` and `PV::Export::ExportCollectionAsMesh`
  are editor-private C++ APIs, and the exporter implementation is in the
  precompiled plugin binary. A reliable repeatable script cannot invoke the
  PVE export from Unreal Python alone. Use the PVE editor export UI for the
  full Beech graph/preset, keeping the export in a disposable test folder; do
  not alter the source graph or Island. The separate Hazel StaticMesh test is
  still independently owned and should not be repurposed for this Beech probe.
- The UE commandlet reached the script's `COMPLETE` marker and exited; the map
  and imported assets were not saved.

## Decision and next step

Do not place these Beech outputs as canopy or scatter them on the Island yet.
Startup Nanite overrides recover leaf geometry, but a greener transient tint
does not fix the sparse silhouette; wind/provider and runtime-cost evidence are
also missing. Next use the PVE editor export UI to create a complete Beech
candidate in an isolated test folder, then capture it in the lit Island
composition. The installed folder contains separate branch, twig, bud, and leaf
instances, so confirm the chosen graph output includes the intended foliage.
Judge it in the fixed Tideglass-to-Woodland Game view and measure a bounded
small batch before any Island placement. Do not raise global foliage density
as a substitute.
