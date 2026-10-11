# Island texture-streaming pool audit — 2026-10-11

## Finding

The project enables texture streaming but does not set a project-specific pool
size in `Config/DefaultEngine.ini`. The UE 5.8.3 process reports
`r.Streaming.PoolSize = 1000`, set by Scalability (High texture quality).
Previous real-RHI Game highlight captures showed the red streaming warning at
about 4.4–5.7 MiB over that pool. This is a small recurring streaming-pool
overage, distinct from the separate whole-GPU video-memory warning recorded in
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
`stat streaming sortby=name maxhistoryframes=1` after streaming settles, then
run `ListTextures` in that same session. Verify the biggest candidates' asset
referencers before changing their resolution, mip policy, or loading behavior.
Keep the current High-quality pool unchanged until target hardware and the
warning view have been measured; do not hide the warning for a performance
claim. Epic documents [`STAT STREAMING`](https://dev.epicgames.com/documentation/en-us/unreal-engine/texture-streaming-metrics-in-unreal-engine)
and the [`ListTextures` runtime inspection](https://dev.epicgames.com/documentation/unreal-engine/texture-asset-editor-in-unreal-engine?lang=en-US).

## Follow-up status

The read-only inventory script now also asks the Asset Registry for direct
hard/soft package referencers of the six largest candidates above. That query is
based on UE's `AssetRegistry.get_referencers` API, but this addition has **not
yet been executed in Unreal**: on 2026-10-11 the editor and Unreal MCP endpoint
were unavailable, and a fresh `UnrealEditor-Cmd` attempt exited before creating
its requested log. A separate `-version` startup probe entered UBT platform
validation, made no CPU progress for over 20 seconds, and produced no new
project log; only that probe and its stalled child processes were stopped. The
candidate referencer list and exact-view `stat streaming` capture therefore
remain pending. No texture settings or assets were changed.
