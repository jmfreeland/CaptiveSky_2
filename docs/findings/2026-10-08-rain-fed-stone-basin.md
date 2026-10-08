# Rain-fed stone basin: deterministic state verified (2026-10-08)

The Listening Stones basin accumulates water in rain and dries faster in sun.
Residents can notice its current water/leaves through their existing nearby
situation summary, then optionally interact to float one leaf per Island day
when there is enough water. Leaves age, the collection is capped at eight, and
the oldest is replaced when a new one arrives. With too little water, a leaf
rests on the stone instead. This extends an existing interaction path; it
doesn't force an action or add a separate model request.

UE 5.8.3 `CaptiveSky_2Editor` compiled the basin code, and
`CaptiveSky2.Agent.IslandRainBasin` and `CaptiveSky2.Agent.IslandRainBasinWorld`
passed together in the scratch editor with agent thinking disabled, model
requests capped at zero, and a 60-second process cap. In addition to the pure
regression for rain/sun bounds, daily interaction, leaf aging/capacity, JSON,
geometry values, and descriptions, the world test now creates a tiny temporary
world, places the basin beside tagged Listening Stones, discovers and interacts
with it through the ordinary resident target path, checks the once-per-day
response, and reloads placement, water, and leaf ownership from an isolated JSON
file in a second world. It also calls the actual resident situation-summary
builder and confirms the exact `RainBasin` affordance appears even without an
`AIslandWeather` actor; basin awareness is now tied to its own subsystem rather
than accidentally nested beneath weather initialization. Local log:
[`Codex_RainBasinResidentContext_20261008.log`](../../Saved/CompileScratch/Codex_RavenCrabAttention_20261008/Saved/Logs/Codex_RainBasinResidentContext_20261008.log).

This verifies deterministic rules and subsystem behavior in a synthetic flat
world, not placement on the saved Island, observed water response to real
weather over time, a resident's choice to act on the affordance, or whether the
basin and floating leaves read clearly at normal gameplay distance. The
synthetic world also lacks the three Listening Stones presentation proxies, so
its expected “presentation skipped” warning is fixture-only. The connected
editor has unsaved Rhododendron material work; leave that session alone. The
next check remains a short PIE observation after its owner has preserved those
edits.
