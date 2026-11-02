# Visitor sky observation — 2026-11-02

## Change

When a visitor uses `E` without a nearby interaction target, the existing
environmental-note cycle now offers a sky observation when there is something
to notice: a seasonal dawn/dusk transition and its cloud-colored horizon, or
the current night-sky visibility. It reuses `AIslandDayNight`'s established
descriptions and the environment subsystem's live cloud and indoor values.

This is an on-demand observation only. It adds no per-frame actor scan, model
call, memory write, or world-state change. Outdoor sky descriptions are not
offered while the visitor is under the inn roof; midday does not falsely claim
that stars are visible.

## Validation

`CaptiveSky2.Agent.VisitorSkyObservation` covers a clear night, indoor
suppression, the clock's seasonally computed sunrise and horizon color, and a
midday negative case. The UE 5.8.3 `CaptiveSky_2Editor` Development target
compiled and linked successfully in the C: worktree (14 UBT actions). UBT
reported the existing `StructUtils` deprecation warning; it did not affect the
build.

The focused NullRHI launch did not reach automation discovery. It crashed at
`SecondsSinceStart=0` with `EXCEPTION_ACCESS_VIOLATION` and the known hash
`F405610C114D120D4B6F5ABA901EC469AC9FAA26`. Log:
`Saved/Logs/Codex_VisitorSkyObservation_NullRHI.log`. This is not a test
failure or a pass; do not repeat the same startup recipe until the bootstrap
path is diagnosed or materially changed.

Next: restore a known-good automation startup path, run the focused test, then
verify one caption in bounded PIE during a memory-gated sky/landscape playtest.
The feature improves discovery of sky work already present; it does not claim
a rendered or visitor-scale art pass.
