# Mixed-canopy shelter clues for residents — 2026-11-01

## Finding

Raven's overhead shelter score and the read-only Rat hollow site audit only
considered spruce crowns. That made the characters' shelter model narrower
than the Island's current ecology, which includes Norway maple, European
aspen, European beech, black alder, and Common Hazel canopies.

## Change

- Generalized the canopy query to inspect mature spruce and all available
  broadleaf A/B instanced-skinned-mesh components. It retains the existing
  collisionless, upper-crown envelope and 900 cm mature-height threshold;
  saplings and understory do not become artificial roofs.
- Raven's weather-aware perch scoring now treats broadleaf crowns as shelter
  clues as well as spruce.
- `Island.RatHollowSiteAudit` now counts nearby mature canopy trees from both
  tree systems for distance/ranking, tests the same mixed canopy for footprint
  cover, and labels its nearest tree accurately. The audit remains read-only;
  its results are still candidate suggestions, not permission to place a home.
- Added the focused `CaptiveSky2.Agent.TreeCrownProbeGeometry` automation. It
  tests broadleaf-shaped upper-crown coverage, rejects trunk-height and
  outside-crown probes, and requires no Island content assets.

## Validation

- UE 5.8.3 `CaptiveSky_2Editor` compiled and linked successfully in the C:
  worktree (4 UBT actions for the focused test update; preceding integration
  build compiled the changed query, audit, Raven controller, and tests).
- `CaptiveSky2.Agent.TreeCrownProbeGeometry` passed under `-NullRHI` with exit
  code 0. The isolated log is
  `Saved/Logs/Codex_MixedCanopy_Geometry_20261101.log`.
- Two broader existing test fixtures could not validate runtime integration
  from this checkout: `GroundCover` dereferenced a missing Island spruce mesh
  at its pre-existing fixture setup, and `RavenPerch` failed because
  `/Game/Agents/BP_Raven_Placeholder` is not mounted here. These are content
  availability limits, not passing tests; the corresponding attempts are
  recorded in `Saved/Logs/Codex_MixedCanopy_GroundCover_20261101.log` and
  `Saved/Logs/Codex_MixedCanopy_RavenPerch_20261101.log`.
- No map was opened or saved, no source was copied into the divergent D:
  checkout, no rendered game was started, and the live editor was left alone.

## Next

After an intentional source merge into the asset-bearing live project and a
safe editor reload, inspect the runtime tree instance counts and run a fresh
Rat hollow site audit in the selected fallen-wood region. Then confirm Raven
can prefer a sheltered perch in an actual light shower. Keep rendered
performance testing behind the established GPU-headroom and 30 FPS gates.
