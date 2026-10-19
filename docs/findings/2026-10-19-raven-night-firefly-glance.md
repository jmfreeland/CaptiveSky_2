# Raven night-firefly glance

Added a small nonverbal wildlife cue: a settled Raven may briefly glance toward a nearby visible firefly once its natural night glow becomes prominent. It reuses the Raven's existing wildlife attention duration and per-nearby-group notice latch. It does not move the Raven, alter the firefly, or issue an LLM request.

The cue requires an Island clock, positive configured firefly glow, clear visibility, a prominent night glow (`DaylightGlowScale >= 0.82`), distance at most 700 cm, and vertical separation at most 250 cm. Daytime/dim fireflies, distant insects, and invalid distances are rejected. The existing wildlife selection still gives priority to a closer eligible cue.

## Verification

- `git diff --check` passed.
- UE 5.8.3 `CaptiveSky_2Editor` scratch-target build succeeded (29 actions; includes `RavenAgentAIController.cpp` and `RavenPerchTests.cpp`). The main editor and its loaded binaries were not modified.
- Added deterministic Raven-perch assertions for night prominence, distance, height, and non-finite input. Unreal automation was not run in this turn; compile success is not a test-pass or in-world visual confirmation.
