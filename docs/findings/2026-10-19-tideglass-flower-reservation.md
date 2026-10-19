# Tideglass specialist flower reservation (2026-10-19)

## Change

The general meadow scatter runs before the eight deterministic Tideglass
pollinator sites and already rejects candidates that are closer than 475 cm to
another flower. It did not reserve that same radius around the later specialist
sites, so generic flowers could pre-empt the locations intended for marsh
marigold and yellow-flag iris.

`AIslandWeather::TryPlaceMeadowFlower` now rejects only general (non-Tideglass)
flower candidates within 475 cm of any deterministic specialist site. The
specialist placements themselves are unchanged. The shared clearance constant
also now defines the normal minimum flower spacing, so those two rules cannot
quietly drift apart.

## Validation

- UE 5.8.3 `CaptiveSky_2Editor` built successfully in the isolated
  `Codex_TideglassPollinator_20261019` scratch project with one worker. The
  first incremental build completed 36 actions; after tightening the focused
  floating-point boundary regression, the rebuild completed four actions.
- `CaptiveSky2.Agent.MeadowFlowerSpecies` passed under NullRHI with thinking
  disabled and zero model requests. The regression covers all eight seeded
  sites across five representative seeds, candidates just inside and outside
  the clearance, and invalid-position handling.
- Log: `Saved/Logs/Codex_TideglassFlowerReservation_20261019.log` (exit code 0).
- The live editor and saved Island were not relaunched or modified. This source
  change removes pre-emption by the earlier general meadow scatter, but does
  not prove that landscape traces, trails, landmarks, existing vegetation, or
  other runtime gates will accept all eight sites. In-world counts and
  appearance remain unverified.
