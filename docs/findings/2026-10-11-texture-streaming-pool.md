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

`Scripts/Inspect-IslandTextureStreaming.py` loaded `/Game/Maps/Island` in an
isolated UE 5.8.3 commandlet, printed `r.Streaming.PoolSize`, then ran the engine
`ListTextures` command. The script did not save the map or assets. Output:
`Saved/Logs/Codex_TextureInventory_20261011.log`.

The largest loaded entries in that inventory were:

- `/Game/PN_FoliageCollection/Textures/FlowerTextures/MasterTextures/Combined_Flowers_A` — 8192×8192, 32 MiB, one mip, `Streaming=NO`, inventory usage count 0.
- `/Game/PN_FoliageCollection/Textures/grassTextures/MasterTextures/springGrass_Opacity_8k` — 8192×8192, 32 MiB, one mip, `Streaming=NO`, inventory usage count 0.
- `/Game/PN_interactiveSpruceForest/Textures/Imposter/half_01_imposter_N` — 4064×4064, 16 MiB, one mip, `Streaming=NO`, inventory usage count 0.
- `/Game/PN_interactiveSpruceForest/Textures/Imposter/half_01_imposter_A`, `.../half_01_imposter_O`, and `/Game/PN_interactiveSpruceForest/Textures/MasterTextures/ModelTextures/Leaf_O` — about 8 MiB each, one mip, `Streaming=NO`, inventory usage count 0.

These are plausible memory candidates, not a proven cause of the small
overage: this inventory was not captured from the warning camera, and
`ListTextures`' usage count is not proof that a texture is safe to delete. No
texture asset settings, source files, project pool settings, or map content
were changed.

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

The read-only inventory script now also asks the Asset Registry for direct
hard/soft package referencers of the six largest candidates above. That query is
based on UE's `AssetRegistry.get_referencers` API, but this addition has **not
yet been executed in Unreal**. The last checked Unreal MCP endpoint was
unavailable and no editor process was running. A fresh `UnrealEditor-Cmd`
attempt had exited before creating its requested log; a separate `-version`
startup probe entered UBT platform validation, made no CPU progress for over
20 seconds, and produced no new project log, so only that probe and its stalled
child processes were stopped. The candidate referencer list and exact-view
`stat streaming` capture therefore remain pending. No texture assets or map
content were changed; the 4096 MiB project pool setting is already committed.
