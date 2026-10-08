# Bounded Raven observation probe (2026-10-08)

The isolated `Island.MoveProbe` command now accepts two optional time controls:

- `DelaySeconds=0..30` waits before resolving/dispatching the action, leaving the game camera time to settle.
- `HoldSeconds=0..20` keeps a successful probe's world alive for observation before the diagnostic requests exit. Failed probes still exit immediately.

For the Tideglass minnow check, the intended invocation is:

```text
Island.MoveProbe Agent_Raven_01 Wander Curious -99180 100060 3020 DelaySeconds=4 HoldSeconds=8
```

The existing `-CaptiveSkyDisableAgentThinking`, isolated data root, and bounded realtime/model-request flags remain required for a safe world probe. The delay and hold are world-time seconds; they are not additional real-time process caps.

## Validation

- UE 5.8.3 `CaptiveSky_2Editor` scratch build succeeded (46 actions), including `IslandMovementProbe.cpp` and the current shared working-tree changes. It used the separate `Saved/CompileScratch/Claude_Props` Binaries with `-NoHotReloadFromIDE`, leaving the open editor's loaded module untouched.
- `CaptiveSky2.Agent.TidepoolMinnows` passed. Its test log records the MinnowSchool curiosity response and confirms the fish remain wild and uncaught.
- `CaptiveSky2.Agent.RavenPerch` passed, including the perch/flight action checks.
- Logs: `Saved/Logs/Codex_TideglassAndRavenAutomation_20261008.log` and `Saved/Logs/Codex_RavenPerchAutomation_20261008.log`.
- The first visible standalone run completed a 1,319 cm Raven flight but crashed during shutdown after the 8-second hold. Its call stack ended in the post-success timer lambda releasing a captured `TSharedRef<FProbeState>`. The timer now uses a capture-free exit callback.
- After that fix, the scratch target rebuilt successfully (4 actions). A second bounded run started at Z=3020 cm, completed a 1,775 cm curious flight in 4.7 simulated seconds, held the view for 8 seconds, and shut down cleanly. It wrote four Tideglass screenshots under `Saved/Playtests/Codex_TideglassDelayedRavenRetry_20261008/Screenshots/` with no subsequent access violation. Model thinking remained disabled and the process cap was 60 real seconds.
- Runtime logs: `Saved/Logs/Codex_TideglassDelayedRaven_20261008.log` (the shutdown crash) and `Saved/Logs/Codex_TideglassDelayedRavenRetry_20261008.log` (clean recheck).

## Still unverified

The hold path has now been exercised through a clean standalone shutdown. The screenshots show the active Tideglass pool and minnows, but the camera does not make a scatter response unmistakable, and the standalone log has no explicit minnow-disturbance event. The automation test does verify that a low flyby scatters and regroups the school. Treat natural Raven-triggered scattering in the full runtime as not yet visually confirmed; the next useful step is a closer, lower flyby capture or temporary probe telemetry, not another claim based on the flight-completion log alone.
