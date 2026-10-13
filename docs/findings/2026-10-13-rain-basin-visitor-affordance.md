# Rain basin visitor affordance

The rain basin already supports a lasting, weather-driven water level and lets a resident or visitor set a leaf afloat once per Island day when the pool reaches `FIslandBasinState::FloatLevel` (0.2). The visitor's nearby `E`/`Interact` hint previously fell through to the generic `inspect RainBasin`, so it hid the available interaction.

The player hint now reflects the basin's current state:

- At or above the float threshold: `float a leaf on the rainwater`.
- Below the threshold: `inspect the rain basin`.

The ordinary interaction path is unchanged: it still owns reach/visibility checks, cooldowns, per-day deduplication, and the durable leaf record. The hint only improves discoverability; it does not cause an interaction or change basin state.

## Verification

- UE 5.8.3 `CaptiveSky_2Editor` Development build succeeded (9 actions, including both changed translation units).
- `CaptiveSky2.Agent.IslandRainBasinWorld` passed under `UnrealEditor-Cmd -NullRHI`; its visitor-controller fixture checks both water-level hints in the synthetic basin world.
- This is behavior evidence for the prompt logic, not a visual review of the basin in the Island map.

## Next

When the editor is available for a visual pass, confirm the hint is legible and appropriately timed beside the basin. For future environmental interactions, prefer exposing existing actions through accurate visitor hints before adding new input paths.
