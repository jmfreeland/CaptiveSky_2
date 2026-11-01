# Rain-basin action-specific cooldown

## Observation

In the isolated 900-second resident session, Raven noticed that the rain basin
was full and chose to set a leaf afloat. Raven was already within two metres,
but the action returned “Already inspected that place recently”; the situation
said 32 seconds remained on the target's generic inspection delay. This
prevented a valid, explicit interaction even though the basin's own rule is
one leaf per resident per Island day.

## Change

`AAutonomousAgentAIController::GetInspectionCooldownSeconds` now returns zero
for `RainBasin`, which already has its own durable per-resident/day limit. The
basin can therefore respond to a ready leaf choice after a recent factual
inspection. A second attempt the same day remains idempotently rejected by the
basin subsystem. Ordinary factual landmarks keep the five-minute delay; the
reversible hearth keeps its one-minute delay.

## Evidence and validation

`FIslandRainBasinWorldTest` now seeds a 32-second generic cooldown, performs an
actual resident-controller interaction, verifies one leaf is saved, then
verifies that a second same-day attempt adds no leaf. Existing basin persistence
and per-day checks remain in the same test. The UE 5.8.3 scratch Editor target
compiled and linked successfully with Live Coding patching disabled; the open
D: editor was not modified. The most recent commandlet launch in this
environment reproduced the known UE bootstrap crash before test startup
(`SecondsSinceStart=0`, hash `F405610C114D120D4B6F5ABA901EC469AC9FAA26`), so I
did not repeat the identical startup after this build. The new world-level
test has not run yet.

## Next check

After the UE commandlet bootstrap issue is diagnosed or the startup environment
meaningfully changes, run `CaptiveSky2.Agent.IslandRainBasinWorld` under
`-NullRHI` with agent thinking disabled and zero model requests. Then verify in
a bounded resident session that an explicitly selected leaf interaction is
completed and the per-resident daily limit is respected.
