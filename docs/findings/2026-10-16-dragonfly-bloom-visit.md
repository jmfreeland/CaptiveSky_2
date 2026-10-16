# Tideglass dragonfly bloom visit — 2026-10-16

The existing bloom response targeted a point 100 cm above the flower instance
pivot and lasted 4.8 seconds. The current source test verified that timed
response, but the available Tideglass captures did not establish that a visitor
could read it as a flower visit rather than ordinary patrol. The target is now
70 cm above the same bloom and the pause lasts 6 seconds (with its fade beginning
at 4.8 seconds). The existing three color morphs retain their separate 32 cm
stations; rainfall suppression, visitor startle, pool-ripple priority, and the
16-second per-dragonfly cooldown are unchanged. This is a visual hypothesis,
not yet an in-world result: no flower instances are added or altered.

The UE 5.8.3 `CaptiveSky_2Editor` target compiled and linked successfully after
the change. The updated `IslandEcologyTests.cpp` compiled, and
`CaptiveSky2.Agent.NightEcology` passed on 2026-10-16 in the existing isolated
scratch project. The run used `-NullRHI`, disabled agent
thinking and Python, capped model requests at zero and realtime at 60 seconds,
and used a separate data root. Log:
[`Codex_DragonflyBloom_20261016.log`](../../Saved/Logs/Codex_DragonflyBloom_20261016.log).
This verifies the behavior contract but not the rendered result.

A separate real-RHI attempt used that scratch project, the fixed Tideglass
spectator camera, a 60-second cap, zero model requests, and flower-visit
telemetry. It exited during engine startup, before the Island or spectator
initialized; the log has no `LogIslandSpectator` or `FlowerVisit` events and no
screenshots were produced:
[`Codex_DragonflyBloomGameplay_20261016.log`](../../Saved/Logs/Codex_DragonflyBloomGameplay_20261016.log).
No new crash-context dump was written, so this attempt does not diagnose the
startup failure. I did not retry it. The earlier main-project startup finding
also remains unresolved; see the [startup minidump finding](2026-10-16-project-startup-minidump.md).

The next validation is a bounded, zero-model-request Tideglass Game capture at
midday with `CaptiveSky.Island.DragonflyFlowerTelemetry=1`, after Unreal can
start the real-RHI Island session. Confirm a
`FlowerVisit started`/`completed` pair and capture the dragonfly beside an
actual meadow flower throughout the pause. Check that its body clears the
flower mesh and that the six-second visit reads at human scale. If it clips or
still reads as patrol, measure the flower's visible tip and adjust only the
hover height after inspection. Keep any play/capture within the existing
30-minute safety cap.
