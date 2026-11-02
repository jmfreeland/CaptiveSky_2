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
