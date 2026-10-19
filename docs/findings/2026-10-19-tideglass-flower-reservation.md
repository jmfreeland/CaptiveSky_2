# Tideglass specialist flower reservation (2026-10-19)

## Change

Both the Rhododendron accents around spruce groves and the general meadow
scatter run before the eight deterministic Tideglass pollinator sites. The
general scatter already rejects candidates that are closer than 475 cm to
another flower, but neither earlier layer originally reserved that same radius
around the later specialist sites. Either layer could therefore pre-empt the
locations intended for marsh marigold and yellow-flag iris.

The Rhododendron grove scatter and `AIslandWeather::TryPlaceMeadowFlower` now
reject non-specialist candidates within 475 cm of any deterministic specialist
site. The specialist placements themselves are unchanged. The shared
clearance constant also defines normal minimum flower spacing, so those rules
cannot quietly drift apart.

## Validation

- UE 5.8.3 `CaptiveSky_2Editor` built successfully in the isolated
  `Codex_TideglassPollinator_20261019` scratch project with one worker. The
  first incremental build completed 36 actions; after tightening the focused
  floating-point boundary regression, the rebuild completed four actions.
- `CaptiveSky2.Agent.MeadowFlowerSpecies` passed under NullRHI with thinking
  disabled and zero model requests. The regression covers all eight seeded
  sites across five representative seeds, candidates just inside and outside
  the clearance, and invalid-position handling.
- After extending the reservation to the earlier Rhododendron accents, the
  isolated target rebuilt successfully in five actions and the same focused
  automation passed again (exit code 0). Latest log:
  `Saved/Logs/Codex_TideglassAllFlowersReservation_20261019.log`.
- Initial focused run log:
  `Saved/Logs/Codex_TideglassFlowerReservation_20261019.log` (exit code 0).
- The live editor and saved Island were not relaunched or modified. This source
  change removes pre-emption by the two earlier flower layers, but does not
  prove that landscape traces, trails, landmarks, existing non-flower
  vegetation, or other runtime gates will accept all eight sites. In-world
  counts and appearance remain unverified.
