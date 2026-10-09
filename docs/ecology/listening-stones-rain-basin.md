# Listening Stones rain basin

The basin's water level now dries faster in wind as well as sunlight. It uses
the existing Island wind magnitude (cm/s) and a bounded drying multiplier that
rises from 1× in calm air to 2× at 600 cm/s or above. Rain continues to refill
the hollow, including during strong wind; wind only changes evaporation. With
zero wind, the existing water-level curve is unchanged.

This is a small local weather response: no new actor, tick, model request, or
saved field was added. The resulting water level still follows the basin's
existing JSON persistence. No authored map or Content asset was changed.

Validation: UE 5.8.3 Editor target build succeeded. Both
`CaptiveSky2.Agent.IslandRainBasin` and `CaptiveSky2.Agent.IslandRainBasinWorld`
passed in an isolated NullRHI automation run (exit code 0); no model requests
were enabled. This verifies the wind/drying curve and basin world/persistence
regressions, not rendered visuals or performance.
