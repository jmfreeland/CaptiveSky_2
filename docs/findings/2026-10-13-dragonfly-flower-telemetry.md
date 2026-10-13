# Dragonfly flower-visit telemetry (2026-10-13)

The daytime Tideglass dragonfly already pauses over nearby meadow flowers for
4.8 seconds, with a 16-second per-insect cooldown. Existing ecology tests cover
entry, the readable hover, fade-out, cooldown, and interruption by a quiet
observer or water ripple. Fixed screenshots every five seconds can miss the
entire pause, so the in-world behavior's visual legibility remained uncertain.

Added the opt-in console variable
`CaptiveSky.Island.DragonflyFlowerTelemetry` (default `0`). Set it to `1` for a
bounded review to log only event boundaries: visit start, completion, or
interruption. Each record includes game time, dragonfly/color variant, flower
hover point, and insect position. The feature adds no per-frame log calls, model
requests, persistence, or change to insect behavior when disabled. For a future
diagnostic run it can be enabled with
`-ExecCmds="CaptiveSky.Island.DragonflyFlowerTelemetry 1"`.

Validation: the UE 5.8.3 `CaptiveSky_2Editor` Development target built
successfully (10 actions; `IslandTideglassDragonfly.cpp` compiled). No Editor or
Game process was started and the existing ecology automation test was not
rerun: project startup still has an unsymbolized null-access crash. Keep the
bounded zero-model-request playtest gate. After matching UE editor symbols are
installed and startup is diagnosed, correlate these timestamps with a close,
bloom-level capture to confirm the pause reads clearly in the actual Island.
