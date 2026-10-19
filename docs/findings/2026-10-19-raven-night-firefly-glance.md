# Raven night-firefly glance

Added a small nonverbal wildlife cue: a settled Raven may briefly glance toward a nearby visible firefly once its natural night glow becomes prominent. It reuses the Raven's existing wildlife attention duration and per-nearby-group notice latch. It does not move the Raven, alter the firefly, or issue an LLM request.

The cue requires an Island clock, positive configured firefly glow, clear visibility, a prominent night glow (`DaylightGlowScale >= 0.82`), distance at most 700 cm, and vertical separation at most 250 cm. Daytime/dim fireflies, distant insects, and invalid distances are rejected. The existing wildlife selection still gives priority to a closer eligible cue.

## Verification

- `git diff --check` passed.
- UE 5.8.3 `CaptiveSky_2Editor` scratch-target build succeeded (29 actions; includes `RavenAgentAIController.cpp` and `RavenPerchTests.cpp`). The main editor and its loaded binaries were not modified.
- Deterministic Raven-perch assertions cover night prominence, distance, height, and non-finite input. The focused `CaptiveSky2.Agent.RavenPerch` automation subsequently passed in the isolated project with `-NullRHI`, thinking disabled, `-CaptiveSkyMaxRealtimeSeconds=60`, and a zero-model-request cap. Log: `Saved/CompileScratch/Codex_TideglassPollinator_20261019/Project/Saved/Logs/Codex_RavenNightFirefly_20261019.log`.
- The test log warns that two skinned-asset compile estimates (4.5 GiB each) exceeded the commandlet's reported 2.1 GiB asset-compile memory limit, but the run completed with exit code 0; available physical memory afterward was 6.2 GiB. This remains deterministic automation evidence only, not an in-world/nighttime visual confirmation.

## Follow-up: sustained insect glance

A code-path audit found that the Raven shortened attention to 0.4 seconds on its next tick for dragonflies, frogs, and fireflies: only stags and foxes were recognized as still-eligible wildlife. It also retargeted all wildlife to a point 85 cm above the actor, too high for small insects. The controller now keeps those three small-species cues for their configured 1.6-second duration while the animal stays nearby, and follows the frog body at +5 cm or insect body at +12 cm. This does not move either animal or add requests.

Verification: UE 5.8.3 incremental scratch build succeeded (5 actions, one worker); the updated `CaptiveSky2.Agent.RavenPerch` automation passed with NullRHI, thinking disabled, a 60-second cap, and zero model requests. New assertions confirm the nighttime firefly target survives a 0.25-second controller tick and its look point tracks the firefly body. Log: `Saved/CompileScratch/Codex_TideglassPollinator_20261019/Project/Saved/Logs/Codex_RavenAttentionPersistence_20261019.log`. A scratch-local HTTP listener reported that `127.0.0.1:8000` was already in use, but the test completed with exit code 0. The live editor/map remain untouched and the cue still needs a real-RHI nighttime visual review.
