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
