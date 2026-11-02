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
compiled and linked successfully in the C: worktree. The final incremental
build compiled the added automation fixture and linked the module in 8.43
seconds. UBT reported the existing `StructUtils` deprecation warning; it did
not affect the build.

The focused NullRHI automation **passed** with exit code 0. Engine
initialization took 54.10 seconds, then test discovery found exactly the
requested test and reported `Test Completed. Result={Success}`. Log:
`Saved/Logs/Codex_VisitorSkyObservation_NullRHI.log`. An `_0000`
`CrashContext.runtime-xml` containing the known zero-second hash was stale; its
last-write time was 00:47 UTC, before this 01:12 UTC passing run. Do not use that
stale artifact as evidence that this invocation crashed. The passing run
establishes the path for this focused automation, but does not explain every
reported Unreal/.NET startup exception.

Next: verify one caption in bounded PIE during a memory-gated sky/landscape
playtest. The feature improves discovery of sky work already present; it does
not claim a rendered or visitor-scale art pass.
