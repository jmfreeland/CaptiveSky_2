# Dragonflies soften their flight in strong wind (2026-10-18)

Tideglass dragonflies already responded to rain and received a bounded drift
from local wind, but their self-powered patrol and wingbeat stayed at full pace
through the strongest gusts. They now gradually soften both as horizontal wind
increases: no change in calm air, a smooth transition beginning at 25 cm/s, and
a nonzero 0.55 movement floor at 180 cm/s and above. Wind-driven displacement
remains independent, so a gust still visibly carries them rather than freezing
their flight. Non-finite wind samples safely preserve the calm-air scale.

The change is local to the dragonfly and does not alter weather sampling,
resident decision frequency, saved world state, or asset content. The ecology
automation now checks calm, breeze, gust, upper-bound, and invalid-input cases.

## Validation and limits

- The UE 5.8.3 `CaptiveSky_2Editor` target built and linked successfully through
  the isolated `Codex_VisitorResonance_20261017` scratch project. This compiled
  `IslandTideglassDragonfly.cpp` and `IslandEcologyTests.cpp` without modifying
  the main project's binaries.
- The focused automation and an in-game visual comparison were not run. The
  project has a previously recorded full-content commandlet startup crash, and
  the current desktop connection exposes no UnrealEditor process. The code is
  compile-verified, not behavior- or presentation-verified.
- On the next safe Game/PIE review, compare a calm and stormy Tideglass flight
  view with zero model requests, checking that the wings and self-powered
  patrol visibly slow while local gust drift remains apparent.
