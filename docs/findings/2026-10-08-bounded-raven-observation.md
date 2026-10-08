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
- A read-only telemetry pass then resolved the apparent false negative: the minnow-school actor is at `(-99500, 100300, 2914)`, 400 cm horizontally from the `TideglassPool` camera marker. The Raven entered the authored low-flyby envelope at 400 cm horizontal / 350 cm vertical, remained in it for 10 samples, and the five fish-body components' relative centroid shifted up to 209 cm from its pre-flight baseline. That displacement matches the school scatter's configured 210 cm fan distance. The 1,985 cm flight completed in 4.4 simulated seconds and the 8-second observation hold ended with a normal UE shutdown.
- The diagnostic samples at 10 Hz only for forced-curiosity probes; it reads Raven position/state and minnow body component locations, and does not modify either resident or the school. Its runtime log is `Saved/Logs/Codex_RavenMinnowTelemetry_20261008.log`; the four before/during/after images are in `Saved/Playtests/Codex_TideglassTelemetry_20261008/Screenshots/`.
- A dedicated waterline-view pass, anchored to `MinnowSchool` rather than the nearby pool marker, reproduced the response: 11 low-flyby samples and 204 cm maximum body-centroid shift, with a clean 60-second-capped run. Its four 1600x900 images are in `Saved/Playtests/Codex_TideglassWaterline_20261008/Screenshots/`. The first angle still left the fish small in frame.
- A closer camera preset made individual fish much easier to read against the water; its images are in `Saved/Playtests/Codex_TideglassWaterline_Close_20261008/Screenshots/`. This second pass sampled only five low-flyby frames and measured 17 cm maximum shift, so it is visual framing evidence, not a replacement for the 204 cm behavior result. Both runs shut down normally and had thinking disabled, isolated world-state directories, a 60-second process cap, and a one-request ceiling.
- The first sandboxed launch failed before map startup: `LogTargetPlatformManager` reported UBT AutoSDK return code `-532462766` (the signed decimal representation of `0xE0434352`), then the shader compiler could not create a transfer file under `C:\Users\freel\UnrealShaderWorkingDir` and UE exited. With approved per-user cache access, AutoSDK returned `0` and the bounded run completed without the shader-transfer failure. This is a strong lead for the recurring `dotnet.exe` popup, but the matching Windows .NET Runtime event/stack was not available, so the exact popup source remains unconfirmed.

## Still unverified

The runtime now has evidence of a natural Raven-triggered minnow response: the actual flight entered the live school's trigger envelope, and fish-body centroid displacement reached the configured scatter distance during that pass. The automation test independently verifies scatter and regrouping. The closer waterline image makes fish readable, but the current small-school scatter is still subtle in a single frame; a surface cue synchronized to the response (for example a brief, low-amplitude ripple) would make the cause-and-effect legible without changing the wild/uncaught behavior.
