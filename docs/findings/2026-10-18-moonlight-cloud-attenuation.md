# Cloud-softened moonlight

The shared clock already lets clouds soften direct sunlight and ambient
skylight, but lunar direct light was unaffected. `AIslandDayNight` now applies
a separate bounded transmission to the Moon: clear sky leaves it unchanged;
full cloud cover transmits 35%, blended linearly between those endpoints. The
new factor composes with lunar phase and the night ramp, so it cannot create
daylight or change the moon's cycle. `IslandDayNightTests.cpp` covers the
transmission bounds/monotonicity and verifies that the actual Moon component is
dimmed by the fixture's overcast weather seed relative to its clear seed.

## Validation

- UE 5.8.3 isolated `Codex_VisitorResonance_20261017` scratch build succeeded
  and linked (30 actions), including `IslandDayNight.cpp` and
  `IslandDayNightTests.cpp`.
- `CaptiveSky2.Agent.DayNight` was compiled but not run. Full-Content commandlet
  startup still has the known Core null-access crash recorded in
  `COORDINATION.md`; do not repeat that launch until startup is fixed.
- No Content or map asset changed. Clear/full-moon and overcast/full-moon
  appearance and night readability still need a real-RHI in-world comparison
  when the editor is responsive.
