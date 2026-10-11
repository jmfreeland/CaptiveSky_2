# Island texture streaming budget

## Change

The project previously inherited Unreal's 1,000 MB texture streaming pool. The
Island combines dense foliage and high-resolution Fab/Megascans textures, and the
user reports recurring over-budget warnings. Set the desktop project pool to
4,096 MB and enabled `r.Streaming.LimitPoolSizeToVRAM` so Unreal can clamp it to
the detected GPU budget on lower-memory cards. Texture streaming remains enabled.

## Verification

- Unreal 5.8.3 real-RHI viewpoint capture loaded `r.Streaming.PoolSize=4096` and
  `r.Streaming.LimitPoolSizeToVRAM=1` from project config.
- The run reported an estimated 7,074 MB texture pool budget (70% of 10,106 MB
  GPU memory) and an active 4,096 MB pool.
- `CaptiveSky2.Visual.Viewpoints` passed at 11:00 with the Tideglass dragonfly
  setup. Capture output: `Saved/Viewpoints/2026-10-11_183335_h11.0/`;
  run log: `Saved/Logs/Codex_TexturePool_Verify_1100_20261011.log`.

## Caveat / next check

The original capture did not collect camera-specific stats. A later bounded
real-RHI `stat streaming` run sampled settled Tideglass-ground and Raven-nest
views at the 4096 MiB pool; Required Pool was 993.00 and 1008.65 MiB, with no
over-budget condition in either view. See the [full texture-pool audit](2026-10-11-texture-streaming-pool.md)
for the screenshots, exact metrics, and remaining caveat: the user's warning
view has not been reproduced. If it recurs, compare Required Pool with the
active Streaming Pool there and inspect wanted mip contributors before raising
the cap. Unreal's pool also covers non-streaming textures and other GPU
resources, so avoid setting it to unlimited. See [Texture Streaming
Configuration](https://dev.epicgames.com/documentation/en-us/unreal-engine/texture-streaming-configuration-in-unreal-engine)
and [Texture Streaming Metrics](https://dev.epicgames.com/documentation/en-us/unreal-engine/texture-streaming-metrics-in-unreal-engine).
