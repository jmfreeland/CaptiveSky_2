# Tideglass pollinator flower edge — 2026-10-13

## Finding

Tideglass dragonflies search for an existing meadow bloom within 900 cm, but the
general flower field excludes landmark clearings out to 2,600 cm. Flower pockets
reserved for Listening Stones also obey that Tideglass exclusion. This leaves the
pool-side flower-visit cue without a nearby habitat target in the authored
scatter layout, even though its behavior and manual-fixture test pass.

## Change

`AIslandWeather` now adds up to eight deterministic, three-bloom pockets in a
700–850 cm ring around Tideglass. The existing pool-footprint clearance is still
honored, and pocket members also avoid nearby cattails, flowers, spruce, shrubs,
trails, curios, steep ground and other landmark clearances. These flowers use the
existing eight Fab meadow meshes, stay collisionless, and do not change the broad
meadow candidate budget or global density. If the terrain or clearance checks
reject a site, that site is omitted rather than forcing a placement.

The `CaptiveSky2.Agent.GroundCover` regression test checks deterministic site
offsets, the pool-side ring bounds, the dragonfly's 900 cm sensory limit, and
spacing between the eight candidate sites.

## Verification and remaining work

- UE 5.8 editor target build: **passed**.
- Test source: compiled as part of an earlier build action; automation test itself
  has **not** run in this turn.
- Runtime scatter and dragonfly visit in the saved Island: **not visually
  verified**. The desktop inventory showed no Unreal Editor, and project startup
  is currently failing before world-ready with an unsymbolized null-address
  access violation. Do not repeat the same project launch until that blocker is
  diagnosed or the launch environment meaningfully changes.
- Next: once a safe editor/Game run is possible, inspect the Tideglass edge at
  player height, confirm at least one candidate pocket survives landscape and
  pool clearance, and capture a dragonfly's approach/hover at one-second cadence
  under the existing zero-model-request cap. Adjust the ring only from that
  evidence.
