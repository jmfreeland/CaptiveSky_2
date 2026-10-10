# Listening Stones rain basin

The basin's water level now dries faster in wind as well as sunlight. It uses
the existing Island wind magnitude (cm/s) and a bounded drying multiplier that
rises from 1× in calm air to 2× at 600 cm/s or above. Rain continues to refill
the hollow, including during strong wind; wind only changes evaporation. With
zero wind, the existing water-level curve is unchanged. Leaves residents have
set afloat now drift and turn gently with the existing horizontal wind vector.
Their motion is derived from immutable saved leaf seeds, current wind and world
time: it is still in calm air, deterministic for a given moment, and bounded to
a few centimetres so leaves remain inside the basin.

This is a small local weather response: no new actor, tick, model request, or
saved field was added. Leaf motion updates at 4 Hz only while floating leaves
exist; the resulting water level and leaf ownership still use the basin's
existing JSON persistence. No authored map or Content asset was changed.

Validation: UE 5.8.3 Editor target build succeeded. Both
`CaptiveSky2.Agent.IslandRainBasin` and `CaptiveSky2.Agent.IslandRainBasinWorld`
passed in an isolated NullRHI automation run (exit code 0), including pure
checks for calm stillness, deterministic breezy movement, the displacement
bound, and invalid wind. The synthetic-world regression also reads the actual
instanced leaf transform after calm and strong-wind updates, confirming the
presentation layer applies the bounded motion. The run allowed zero model
requests. This verifies the wind/drying and basin world/persistence behavior,
not rendered visibility or a measured performance change. Log:
[`Codex_BasinLeafDrift_Automation_20261010.log`](../../Saved/Logs/Codex_BasinLeafDrift_Automation_20261010.log).
