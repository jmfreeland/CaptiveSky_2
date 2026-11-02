# Visitor wildlife observation — 2026-11-02

When the visitor has no nearby actionable target, pressing Interact cycles
through on-demand environmental notes. That cycle now includes the nearest
recognized `IslandLife` actor within 1,800 cm when the visitor can see it using
the same visibility check as direct interactions. Supported observations name
the existing stag, fox, firefly, shore crab, periwinkle, Tideglass dragonfly,
common frog, or minnow school. The note says the visitor leaves it
undisturbed. This is a descriptive cue: it does not startle or move an animal,
write memory or world state, or call a model. Discovery occurs only when the
visitor requests an observation.

Validation: the UE 5.8.3 `CaptiveSky_2Editor` Development target compiled and
linked. The focused NullRHI automation
`CaptiveSky2.Agent.VisitorWildlifeObservation` passed (exit code 0), covering
species recognition, the same visibility gate used by interactions, discovery
beyond the 4 m direct-interaction radius, the rotating caption cycle, and
suppression of hidden or distant animals. Log:
`Saved/Logs/Codex_VisitorWildlifeObservation_NullRHI.log`.

This does not prove that each species is visually distinguishable at 18 m or
that the local caption is legible during normal gameplay. Review the note in a
bounded, memory-gated rendered Island session before claiming visitor-scale
readability; retain the existing 30-FPS floor and play/request safeguards.

## Content-linked validation follow-up

The same `VisitorWildlifeObservation` automation also passed in the isolated
`Saved/CS58` project, whose `Content` junction points to the actual Island
assets. The `CaptiveSky2.Agent.IslandEnvironment` test passed there as well,
including loading and publishing the existing `PN_WindParameters` collection.
Its first run exposed a stale test fixture: it asserted golden hour at 17:00
while leaving the Island clock on day 1 (midwinter). The test now pins the
spring-equinox day that its assertion assumes. Neither run loaded or saved the
Island map. Logs:
`Saved/CS58/Saved/Logs/Codex_VisitorWildlifeObservation_RealContent_NullRHI.log`
and `Saved/CS58/Saved/Logs/Codex_IslandEnvironment_RealContent_NullRHI.log`.

A rendered review was deliberately deferred: the RTX 4080 Laptop GPU reported
10,017 / 12,282 MiB in use, leaving 2,265 MiB free—below the established
4,096 MiB render preflight. Recheck the same daylight coastal-to-woodland
route only after that reserve is available; NullRHI success is not visual or
performance evidence.
