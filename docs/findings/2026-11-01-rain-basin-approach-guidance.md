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
