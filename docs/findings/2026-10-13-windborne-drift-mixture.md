# Windborne ecology starts with a mixed population — 2026-10-13

`AIslandDrift::Advance` initially filled active slots in their storage order.
Because the slots are partitioned by species and seeds are first, a fresh breeze
spent its first-frame population on seeds before reaching petals or leaves. This
made the visual response less diverse precisely when wind first became noticeable.

The initial population now builds a list of only the species whose lift threshold
is met, shuffles those slots with the actor's existing deterministic random stream,
then fills the same weather-derived target count. Slot caps, lift thresholds,
wind/rain/daylight gates, subsequent spawn behavior, and persistence are unchanged.

Verification on 2026-10-13:

- UE 5.8.3 `CaptiveSky_2Editor` target built successfully (8 actions; compiled
  `IslandDrift.cpp` and `IslandDriftTests.cpp`).
- `CaptiveSky2.Agent.IslandDrift` passed under NullRHI in a commandlet run with
  resident thinking disabled, Python disabled, zero model requests, a 60-second
  runtime cap, and an isolated world-state root. The test now confirms that the
  first 100 cm/s breeze population includes each lift-eligible mote kind.
- The existing 96-mote hard cap and heavy-rain clearing regression passed in the
  same test.

This proves the bounded selection behavior, not the visual readability of the
cube-based leaf/petal silhouettes in normal play. A real-RHI gameplay look at
ordinary viewing distance remains useful before considering further drift-art
work.

Evidence: [`Codex_IslandDrift_Diversity_20261013.log`](../../Saved/Logs/Codex_IslandDrift_Diversity_20261013.log).
