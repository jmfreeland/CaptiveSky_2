# Shore-crab activity follows Tideglass (2026-10-08)

The Tideglass shore crabs now let their ordinary idle-foraging loop respond to
the existing Island lunar tide. As water recedes, the local drift grows gently
to 1.18x; at high water it contracts to 0.82x. The response is derived from
`AIslandDayNight`'s current hour/day and the same bounded tide function that
moves the transient pool surface. With no tide displacement or no clock, the
scale is exactly the old 1.0 baseline.

Only the crab's ordinary drift is scaled. A Raven/observer avoidance scurry
still uses its existing full displacement and cooldown, and shoreline home
locations, collision handling, sheltering, saving, and character actions are
unchanged. No extra tick, actor search, model call, memory, or world-state field
was added. The tide is sampled by the crab's existing 0.08-second tick.

`CaptiveSky2.Agent.NightEcology` now checks low-water/high-water ordering,
bounded output beyond the tide range, the no-tide baseline, and actual crab
movement at opposite phases while holding world time and idle phase constant.
The UE 5.8.3 scratch editor target built and linked successfully; the focused
NullRHI automation completed with `Result={Success}` and exit code 0. Resident
thinking and Python were disabled, the request cap was zero, the runtime cap
was 60 seconds, and the test wrote only to its scratch data root. The user's
interactive editor and unsaved Island level were not saved, closed, or
modified.

This verifies the bounded behavior path but not how clearly the small drift
difference reads in a normal Game/PIE view. Next visual review should look at a
shore crab during one high-water and one low-water phase before increasing the
range or adding a stronger tidal response.

The full isolated editor target built in 149 actions; after the final multiplier
tuning, the incremental compile and link completed all five actions. The final
test log also records `UBT AutoSDK ReturnCode: 0`.

Log: [`TidepoolCrabTideAutomation_Final.log`](../../Saved/CompileScratch/Codex_TidalCrabActivity_20261008/Project/Saved/Logs/TidepoolCrabTideAutomation_Final.log).
