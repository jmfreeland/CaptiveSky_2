# Rain-basin approach guidance — 2026-11-01

The isolated 15-minute resident run recorded Rat choosing to set a leaf afloat
while outside the basin's four-metre interaction radius. The
action failed immediately with “Too far away to inspect; move within four metres
first.” The basin was full by a later observation, so this run found an
approach-awareness miss, not a failure of the water or persistent-leaf system.

`UIslandRainBasinSubsystem::DescribeNearby` now states the two-step choice
explicitly when a resident is outside interaction range: move to `RainBasin`,
then decide whether to interact after arriving. From within four metres it
describes the leaf effect directly. This preserves resident choice and does not
automatically move or interact on their behalf. Wildlife approach restrictions,
request limits, cooldowns, and the basin's once-per-resident-per-day behavior
are unchanged.

The UE 5.8.3 `CaptiveSky_2Editor` target built successfully against the isolated
`Saved/CS58` scratch project. Both `CaptiveSky2.Agent.IslandRainBasin` and
`CaptiveSky2.Agent.IslandRainBasinWorld` automation tests passed under NullRHI;
the world test now asserts guidance on both sides of the four-metre threshold.
No rendered basin scene or live resident retest was run, so the next 15-minute
feedback pass should check whether Rat approaches first and only floats a leaf
when the basin is wet and he is within reach.

Run log: [`Codex_RainBasinRangeAffordance_20261101.log`](../../Saved/CS58/Saved/Logs/Codex_RainBasinRangeAffordance_20261101.log).

## General interaction rule follow-up

The ordinary resident decision prompt now generalizes the same range contract
to inanimate landmarks and objects: beyond four metres, move to the exact
offered target first and reconsider interaction after arrival. It explicitly
keeps wildlife outside `move_to` and permits wildlife interaction only when
already within four metres. This keeps the failed basin choice from becoming a
repeatable pattern at the other shared landmark affordances without making
movement or interaction automatic.

The UE 5.8.3 scratch Editor target rebuilt successfully after the prompt change.
The focused `IslandRainBasin` and `IslandRainBasinWorld` automation tests both
passed under NullRHI, including an assertion for the global prompt contract.
Run log: [`Codex_RainBasinGlobalRange_20261101.log`](../../Saved/CS58/Saved/Logs/Codex_RainBasinGlobalRange_20261101.log).

## Post-change resident retest attempt

A fresh isolated 900-second NullRHI character-feedback run was attempted with
the new prompt, Python disabled, situation logging enabled, and a 40-request
ceiling. `Start-Spectator.ps1` correctly rejected the process: it exited with
code 0 without producing its normal project log or the Island world-ready
marker. The crash artifact records a game-thread `EXCEPTION_ACCESS_VIOLATION`
reading null at zero seconds since start, process ID 0, with hash
`4DCE2FEABCC3C947BBCB4179F07C8373B60F798D`; its stack contains only unsymbolized
UnrealEditor-Core and UnrealEditor offsets. This matches the known startup
failure signature; the log's Unreal Trace Server error is not evidence of its
cause. No resident behavior or prompt uptake was observed, and the attempt must
not be counted as a playtest.

No local CDB/WinDbg executable or matching UnrealEditor PDB was present at the
checked Windows SDK and UE 5.8.3 engine paths, so this attempt does not provide
a new diagnosis. Do not repeat the same launch path blindly; obtain a
symbolized dump or materially different engine startup path first. The latest
device sample remains below the separate 4 GiB rendered-session gate.

Crash artifact: [`Codex_PostRangePrompt_20261101.log`](../../Saved/CS58/Saved/Crashes/_0000/Codex_PostRangePrompt_20261101.log).
