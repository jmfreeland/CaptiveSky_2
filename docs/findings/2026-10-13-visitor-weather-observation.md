# Visitor weather observation

When no landmark, nearby creature, or readable trail is available, visitors can
now pause to notice active outdoor weather. The prompt appears only when rain,
lingering dampness, meaningful wind, or visible mist is present, and is
suppressed under the inn roof. Landmark interactions remain first, followed by
trail reading and then weather. The text uses the existing environment
subsystem's rain, wetness, wind, mist, and indoor state; it creates no model
request, saved state, timer, or new simulation.

## Verification

The UE 5.8.3 editor target built successfully (15 actions); the controller,
environment subsystem, and new automation test all compiled. The focused
`CaptiveSky2.Agent.VisitorWeatherObservation` test did not run: its commandlet
crashed at `SecondsSinceStart=0` before creating the requested project log or
reaching automation. The crash repeated the known null-address access violation
with stack hash
`F405610C114D120D4B6F5ABA901EC469AC9FAA26` (see
[`Codex_VisitorWeatherObservation_20261013.log`](../../Saved/Crashes/_0000/Codex_VisitorWeatherObservation_20261013.log)
and the adjacent crash context). A zero process exit code is not a test pass.
The compiled automation covers clear conditions, rain, wind, lingering
dampness, mist, controller prompt/caption behavior, and indoor suppression.

## Next

Install matching UE 5.8.3 Editor Symbols for Debugging and diagnose the existing
startup fault before another launch. Once the focused automation can run, make
one bounded zero-model-request Game/PIE review during rain or strong wind at an
exposed viewpoint, then step under the inn roof to confirm the prompt
disappears. This synthetic automation does not prove visual/UI readability or
that the environmental state is accurately sampled during play.
