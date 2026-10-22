# Island reload and bounded runtime check — 2026-10-22

After approval to reload/rebuild the Island, the open editor reloaded
`/Game/Maps/Island` and completed one brief in-editor play start. PIE was stopped
after the first foliage initialization log; the editor returned to the Island in
non-PIE state. No source, map, or content assets were edited by this check.

## Observed state

- After reload, the hidden `OceanPlane` component reported `bVisible=false` and
  `bodyInstance.collisionEnabled=NoCollision`. This confirms the saved plane is
  no longer a walkable collision surface; it does not prove the player's swim
  transition, swimming movement, or return to shore.
- Runtime initialization logged 8,317 valid terrain cells sampled and 84,801
  ground-cover instances placed after 136,002 bounded traces (budget 96,000,
  density 0.05).
- Woodland initialization logged 14,430 trees (8,745 saplings), 15,036 broadleaf
  shrubs, and 1,491 flowering rhododendrons in 500 groves after 52,249 bounded
  traces. This confirms runtime population and grove clustering, not whether the
  distribution reads as natural from player scale.
- The Norway maple, European aspen, and European beech meshes each still logged
  a missing skeletal-mesh material-usage flag warning.
- The open editor reported `r.Nanite.Foliage=0`, despite the project config
  enabling it. The config change therefore still needs an editor restart before
  Nanite-foliage behavior can be evaluated.
- After PIE stopped, the live editor's focused `CaptiveSky2.Agent.IslandOcean`
  and `CaptiveSky2.Agent.IslandWeather` tests both passed (1/1 each). The ocean
  case had no warnings. The weather fixture logged that it intentionally placed
  no ground cover, plus transient-world teardown warnings; those are test-world
  fixture diagnostics, not evidence about the saved Island's runtime scatter.
- No matching video-memory-exhaustion/budget warning appeared in the current
  session log during this brief run. This is not evidence that the prior GPU
  budget warning is resolved: the capture was short, no sustained gameplay or
  GPU telemetry was collected, and the earlier warning was observed separately.

## Next validation

Keep the active material-usage repair and Nanite/config work as the next gates.
After they land and the editor is restarted, capture one bounded, representative
runtime view at player scale; inspect the species mix and grove/understory
transitions, and record GPU memory plus frame-time headroom. Do not use this
brief initialization pass as the vegetation-naturalness or performance sign-off.
