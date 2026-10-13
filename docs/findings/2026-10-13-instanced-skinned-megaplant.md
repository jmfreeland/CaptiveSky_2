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
- The component reports no transform provider. Wind and animated per-instance
  bone transforms therefore remain untested. These one-tree scene captures do
  not measure runtime cost, LOD behavior, or a multi-instance batch.
- The UE commandlet reached the script's `COMPLETE` marker and exited; the map
  and imported assets were not saved.

## Decision and next step

Do not place these Beech outputs as canopy or scatter them on the Island yet.
Startup Nanite overrides recover leaf geometry, but its isolated material and
silhouette still look wrong; there is also no wind/provider or runtime-cost
evidence. Next inspect the Beech PVE graph/preset and material parameters, then
capture one candidate in the lit Island composition. The installed folder
contains separate branch, twig, bud, and leaf instances, so a complete exported
asset may be required. Keep that export isolated, judge it in the fixed
Tideglass-to-Woodland Game view, and measure a bounded small batch before any
Island placement. Do not raise global foliage density as a substitute.
