# Island texture-streaming pool audit — 2026-10-11

## Finding

The project sets `r.Streaming.PoolSize=4096` and
`r.Streaming.LimitPoolSizeToVRAM=1` in `Config/DefaultEngine.ini` (commit
`cacced2`). The saved game profile is Texture Quality 3. UE 5.8.3's engine
`BaseScalability.ini` overrides that profile to a 1000 MiB pool and disables
the VRAM clamp, which explains why the warning could recur in Game despite the
project default. `Config/DefaultScalability.ini` now overrides just those two
High-profile settings, preserving the selected texture quality and other
scalability choices. The Game startup log
(`Saved/Logs/Codex_TextureScalability_MemoryDDC_20261011.log`) sets both project
values and applies `[TextureQuality@3]` without emitting the engine's 1000 MiB
override. It then reports `UBT AutoSDK ReturnCode: -532462766` and never reaches
the Island map; the isolated launch was stopped before gameplay. Thus startup
confirms the override is loaded but the final in-session CVar and camera-specific
pool counters remain unverified.

Follow-up bounded Game run, `Saved/Logs/Codex_TextureScalability_Recheck_20261011.log`,
reached Island after a 434-second first-time local shader/texture build, then
exited at the 30-second cap with thinking disabled and zero model requests. The
runtime applied `[TextureQuality@3]`, set both project CVars, and logged
`Texture pool size now 4096 MB`. No `Required Pool` or `Over Budget` counters
were captured in that run. It confirms the High-profile correction is active;
it does not establish whether the user's recurring warning remains or which
view/assets trigger it. The startup build also reached about 10 GiB of system
working memory while building cached textures, which is separate from the
runtime streaming-pool measurement.

Earlier isolated real-RHI UE 5.8.3 capture logs confirm the project-default
pool was 4096 MiB:
`Saved/Logs/Codex_TexturePool_Verify_20261011.log` and
`Saved/Logs/Codex_TexturePool_Verify_1100_20261011.log`. The RHI reports a
7074 MiB automatically estimated pool ceiling on this machine, but that is not
evidence Island needs that much or that raising the project pool is safe.

Earlier real-RHI Game highlight captures showed the red streaming warning
about 4.4–5.7 MiB over the former 1000 MiB pool. That proves the old setting
was occasionally exceeded; the newer captures confirm the 4096 MiB setting is
active, but do **not** record the warning camera's Required Pool / Over Budget
figures. The user's more recent observation that the warning still recurs is
not yet quantified against the corrected High-profile override. This
streaming-pool warning is distinct from the separate whole-GPU video-memory warning in
`2026-10-08-editor-vram-pressure.md`.

An additional capped, zero-request standalone Game capture on 2026-10-11 sampled
two settled views with `stat streaming` enabled. The High texture profile logged
`Texture pool size now 4096 MB`:

| Settled view | Required Pool | Visible Mips | Hidden Mips | Over Budget |
|---|---:|---:|---:|---:|
| Tideglass ground detail | 993.00 MiB | 943.87 MiB | 42.38 MiB | 0 MiB |
| Raven nest close-up | 1008.64 MiB | 960.14 MiB | 41.74 MiB | 0 MiB |

Screenshots are in
`Saved/Playtests/Codex_TexturePool_Multiview_20261011/Screenshots/` and the
log is `Saved/Logs/Codex_TexturePool_Multiview_20261011.log`. These two views do
not reproduce the user's frequently observed over-budget warning; they are not
representative of every camera path or texture-quality/device configuration.
Both were sampled at 11:14–11:43 game time with the High profile and the 4096
MiB pool. The exact view and profile that show the warning remain the most
useful next evidence.

## Read-only Island inventory

`Scripts/Inspect-IslandTextureStreaming.py` loads `/Game/Maps/Island` in an
isolated UE 5.8.3 commandlet, prints `r.Streaming.PoolSize`, runs `ListTextures`,
reads candidate texture settings, and walks a bounded Asset Registry
referencer graph looking for package paths back to Island. It does not save the
map or assets. Latest output:
`Saved/Logs/Codex_TextureInventory_Settings_20261011.log` (the dependency walk
also ran independently in `Codex_TextureInventory_Transitive_20261011.log`).

The candidates below appeared in `ListTextures` in the map-loaded commandlet
session. The table records the original settings before the local mip trial.
Registry paths prove package dependency, not that a texture is visible in the
current camera or is a cause of the streaming warning:

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

## Reversible grass-opacity mip trial

The Island-referenced grass texture was backed up byte-for-byte at
`Saved/Backups/springGrass_Opacity_8k_PreMips_20261011.uasset` (SHA-256
`747E4D41D8E7A8104F65815A348FB65530918C6AEC9C49897FE42361F955BE6D`). The
guarded one-shot writer is `Scripts/Configure-IslandGrassOpacityMips.py`.
It changed only `springGrass_Opacity_8k` to `TMGS_FROM_TEXTURE_GROUP` with
`MaxTextureSize=4096`; `NeverStream` remains false. The package remains local
under Content (not tracked by Git).

After saving and reloading, UE 5.8.3 reports the texture as 4096×4096 and
10,944 KB, versus the original 8192×8192 and 32,768 KB. The platform texture
footprint is therefore 21,824 KB (about 21.3 MiB) smaller at the listed
resolution, and group-based mip generation now lets lower resolutions stream
as demand falls. This is an asset footprint comparison, not a measurement of
actual `Required Pool` or proof that the recurring warning is fixed.

Matched ground-cover Tideglass captures passed before and after the change.
Visual inspection of the dense close view found no obvious loss in grass
silhouette or mask edges. Scene-capture p95 throughput was 55.43 / 57.73 /
57.73 FPS before and 56.58 / 56.34 / 57.20 FPS after (overview / ground detail
/ dragonfly close), too close to treat as a performance difference. These are
editor capture results, not Game/PIE or target-hardware guarantees. The Island
map was not saved.

Baseline log/images:
`Saved/Logs/Codex_GrassOpacity_Baseline_Viewpoints_Dragonflies_20261011.log`,
`Saved/Viewpoints/2026-10-11_194428_h11.0/`.
Mip-trial log/images:
`Saved/Logs/Codex_GrassOpacity_MipTrial_Viewpoints_20261011.log`,
`Saved/Viewpoints/2026-10-11_194755_h11.0/`.

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

**Warm-cache `stat streaming` capture (2026-10-11):** a standalone Game run
reused the local shader/DDC cache, held the 11:00 Tideglass viewpoint, disabled
resident thinking, and ended at its 60-second cap with zero model requests. The
log is [`Codex_StarterInstanced_StatStreaming_20261011.log`](../../Saved/Logs/Codex_StarterInstanced_StatStreaming_20261011.log);
the stat overlay is visible in
[`002_Tideglass.png`](../../Saved/Playtests/Codex_StarterInstanced_StatStreaming_20261011/Screenshots/002_Tideglass.png).
At this view the streaming pool is 4096 MiB, Required Pool is 990.71 MiB,
Visible Mips 953.48 MiB, Non-streaming Mips 269.09 MiB, Hidden Mips 30.48 MiB,
and Forced/UnknownRef Mips are 0 MiB. No streaming over-budget counter is
shown; Required Pool is about 24% of capacity. The red “PROFILING WITH LOGGING
ON!” banner is part of the diagnostic overlay, not a texture-budget alert.
This establishes that the Tideglass view is comfortably within the corrected
pool; it does not reproduce or disprove the user's reports at other views.

The direct and transitive Asset Registry queries now complete in UE 5.8.3; the
dependency walk found Island paths for the grass opacity texture and all four
spruce maps above, and none for the 8K combined-flower texture. The 8K grass
opacity texture now has a verified 4K mipmapped local asset state and exact
restore point. The user's warning view is still needed for the final diagnosis;
do not yet claim the recurring warning is resolved everywhere. No map content
was changed; the separate 4096 MiB project pool setting remains as committed.
