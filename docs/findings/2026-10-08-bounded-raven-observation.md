# Bounded Raven observation probe (2026-10-08)

The isolated `Island.MoveProbe` command now accepts two optional time controls:

- `DelaySeconds=0..30` waits before resolving/dispatching the action, leaving the game camera time to settle.
- `HoldSeconds=0..20` keeps a successful probe's world alive for observation before the diagnostic requests exit. Failed probes still exit immediately.

For the Tideglass minnow check, the intended invocation is:

```text
Island.MoveProbe Agent_Raven_01 Wander Curious -99180 100060 3524 DelaySeconds=10 HoldSeconds=8
```

The existing `-CaptiveSkyDisableAgentThinking`, isolated data root, and bounded realtime/model-request flags remain required for a safe world probe. The delay and hold are world-time seconds; they are not additional real-time process caps.

## Validation

- UE 5.8.3 `CaptiveSky_2Editor` scratch build succeeded (46 actions), including `IslandMovementProbe.cpp` and the current shared working-tree changes. It used the separate `Saved/CompileScratch/Claude_Props` Binaries with `-NoHotReloadFromIDE`, leaving the open editor's loaded module untouched.
- `CaptiveSky2.Agent.TidepoolMinnows` passed. Its test log records the MinnowSchool curiosity response and confirms the fish remain wild and uncaught.
- `CaptiveSky2.Agent.RavenPerch` passed, including the perch/flight action checks.
- Logs: `Saved/Logs/Codex_TideglassAndRavenAutomation_20261008.log` and `Saved/Logs/Codex_RavenPerchAutomation_20261008.log`.

## Still unverified

The delay/hold options have compiled but have not yet been exercised in a visible standalone game. The earlier bounded Raven flight reached its destination, but no captured runtime image or explicit runtime cue log proved that the school scattered in response. Re-run the invocation above with a 60-second realtime cap and inspect a screenshot during the 8-second hold before treating the interaction as visually verified.
