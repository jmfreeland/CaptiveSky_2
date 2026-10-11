# Island texture-streaming pool audit — 2026-10-11

## Finding

The project now explicitly sets `r.Streaming.PoolSize=4096` and
`r.Streaming.LimitPoolSizeToVRAM=1` in `Config/DefaultEngine.ini` (commit
`cacced2`). Real-RHI UE 5.8.3 capture logs confirm the active streaming pool
changed from the previous scalability default of 1000 MiB to 4096 MiB:
`Saved/Logs/Codex_TexturePool_Verify_20261011.log` and
`Saved/Logs/Codex_TexturePool_Verify_1100_20261011.log`. The RHI reports a
7074 MiB automatically estimated pool ceiling on this machine, but that is not
evidence Island needs that much or that raising the project pool is safe.

Earlier real-RHI Game highlight captures showed the red streaming warning
about 4.4–5.7 MiB over the former 1000 MiB pool. That proves the old setting
was occasionally exceeded; the newer captures confirm the 4096 MiB setting is
active, but do **not** record the warning camera's Required Pool / Over Budget
figures. The user's more recent observation that the warning still recurs is
not yet quantified against the new limit. This streaming-pool warning is
distinct from the separate whole-GPU video-memory warning recorded in
`2026-10-08-editor-vram-pressure.md`.

## Read-only Island inventory

`Scripts/Inspect-IslandTextureStreaming.py` loads `/Game/Maps/Island` in an
isolated UE 5.8.3 commandlet, prints `r.Streaming.PoolSize`, runs `ListTextures`,
reads candidate texture settings, and walks a bounded Asset Registry
referencer graph looking for package paths back to Island. It does not save the
map or assets. Latest output:
`Saved/Logs/Codex_TextureInventory_Settings_20261011.log` (the dependency walk
also ran independently in `Codex_TextureInventory_Transitive_20261011.log`).

The candidates below appeared in `ListTextures` in the map-loaded commandlet
session. Registry paths prove package dependency, not that a texture is visible
in the current camera or is a cause of the streaming warning:

| Candidate | Loaded size / format | Mip and streaming settings | Island dependency path |
|---|---|---|---|
| `PN_FoliageCollection/.../springGrass_Opacity_8k` | 8192², 32 MiB, DXT1 | `TMGS_NO_MIPMAPS`, `NeverStream=false`, not virtual | 3 paths via grass meshes and `/Game/LandscapeGrassTypes/GT_MidLow` (also through the shore Substrate material) |
| `PN_interactiveSpruceForest/.../half_01_imposter_N` | 4064², 16 MiB, BC5 | `TMGS_NO_MIPMAPS`, `NeverStream=true` | Material instance → `spruce_half_01` mesh → Island |
| `PN_interactiveSpruceForest/.../half_01_imposter_A` | 4096², 8 MiB, DXT1 | `TMGS_NO_MIPMAPS`, `NeverStream=true` | Same mesh path |
| `PN_interactiveSpruceForest/.../half_01_imposter_O` | 4064², 8 MiB, DXT1 | `TMGS_NO_MIPMAPS`, `NeverStream=true` | Same mesh path |
| `PN_interactiveSpruceForest/.../ModelTextures/Leaf_O` | 4096², 8 MiB, DXT1 | `TMGS_NO_MIPMAPS`, `NeverStream=false`, not virtual | Leaf material → same mesh → Island |
| `PN_FoliageCollection/.../Combined_Flowers_A` | 8192², 32 MiB, DXT1 | `TMGS_NO_MIPMAPS`, `NeverStream=false`, not virtual | **No path to Island found** (0 paths; 290 packages searched) |

The spruce maps total about 40 MiB loaded, including 32 MiB marked
`NeverStream=true`. The Island package dependency does not identify the actor
that references `spruce_half_01`; the PCG audit separately says the disabled
forest outputs use `/PCG/SampleContent/SimpleForest/Meshes/PCG_Tree_01..03`,
so the two must not be conflated. The 8K grass opacity map is an
Island-reachable residency candidate: it has no mip chain and is referenced
through landscape-grass assets. None of these facts alone
quantifies the camera's Required Pool or proves which asset causes the warning.
No texture settings or map content were changed.

## Next diagnostic

On the exact Island view where the warning appears, capture
`stat streaming sortby=name maxhistoryframes=1` after streaming settles and
record Required Pool and Over Budget at the current 4096 MiB setting. Then run
`ListTextures` in that same session. If actual demand is well below 4096 MiB,
investigate the warning's source/other memory category instead of increasing
this pool; if it exceeds 4096 MiB, verify the top wanted-mip contributors and
their referencers before changing resolution, mip policy, or loading behavior.
Do not hide the warning or raise the pool blindly. Epic documents [`STAT STREAMING`](https://dev.epicgames.com/documentation/en-us/unreal-engine/texture-streaming-metrics-in-unreal-engine)
and the [`ListTextures` runtime inspection](https://dev.epicgames.com/documentation/unreal-engine/texture-asset-editor-in-unreal-engine?lang=en-US).

## Follow-up status

The direct and transitive Asset Registry queries now complete in UE 5.8.3; the
dependency walk found Island paths for the grass opacity texture and all four
spruce maps above, and none for the 8K combined-flower texture. The exact-view
`stat streaming` capture remains pending because it needs the live warning
view. No texture assets or map content were changed; the 4096 MiB project pool
setting is already committed.
