# Island reload: runtime vegetation and ocean collision probe

## What was checked

Reloaded `/Game/Maps/Island` in the open UE 5.8.3 editor and started one short
in-viewport PIE session with an 8-second initialization warmup. PIE was stopped
after confirming the landscape's deferred runtime scatter had completed. No map
or content asset was saved or modified by this check.

## Runtime vegetation evidence

`LogIslandWeather` reported:

- 84,796 ground-cover instances from 8,317 valid terrain cells, after 136,002
  bounded traces (budget 96,000; configured density 0.05).
- 14,430 trees (8,745 saplings), 15,036 broadleaf shrubs, and 1,491 flowering
  rhododendrons across 500 groves.
- 600 Norway maple, 600 European aspen, and 600 European beech instances, plus
  39 birch and 57 hazel coppice instances. The three Megaplant broadleaf species
  were therefore present in the runtime population; a foliage count alone does
  not establish that their materials or silhouettes read well to a player.
- 299 meadow flowers (11 species), including five Tideglass pollinator sites,
  and 10 nonblocking wet-edge cattails.

The short PIE log contained no graphics-memory-exhaustion warning. This was not
a GPU-memory measurement or a sustained traversal/performance test, so it does
not resolve the reported texture/VRAM budget issue. The editor-only viewport
capture is also not a substitute for judging the runtime material path.

## Ocean collision evidence and limits

In the loaded editor world, `WaterBodyOcean_1` has bounds spanning the tested
ocean point at `(200000, 0)`. `SceneTools.trace_world` returned:

- `(200000, 0, 500)` to `(200000, 0, -500)`: hit at 500 cm, i.e. world `Z=0`.
- `(200000, 0, 500)` to `(200000, 0, 100)`: no hit.
- `(200000, 0, -100)` to `(200000, 0, -1000)`: no hit.

This confirms a traceable surface at sea level in open ocean and is consistent
with the reported walk-on-water behavior. It does **not** by itself prove how a
player capsule responds, whether swimming entry supersedes the surface contact,
or what happens at the island shore. A bounded, input-driven PIE crossing is
still needed before changing collision or movement behavior.

## Related validation and next step

Both existing rain-basin tests passed in the live editor:
`CaptiveSky2.Agent.IslandRainBasin` and
`CaptiveSky2.Agent.IslandRainBasinWorld`. The world fixture emitted its known
temporary-world teardown warnings, but no test failures.

Next, perform a short, capped, zero-request player test that crosses from shore
to ocean and back while recording pawn Z/velocity and swim-state transitions.
Keep the existing swim/movement work isolated until its coordination claim is
cleared; avoid saving a water-component change into the gitignored Island map
without explicit user confirmation.
