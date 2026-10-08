# Rain-fed stone basin: deterministic state verified (2026-10-08)

The Listening Stones basin accumulates water in rain and dries faster in sun.
Residents can notice its current water/leaves through their existing nearby
situation summary, then optionally interact to float one leaf per Island day
when there is enough water. Leaves age, the collection is capped at eight, and
the oldest is replaced when a new one arrives. With too little water, a leaf
rests on the stone instead. This extends an existing interaction path; it
doesn't force an action or add a separate model request.

UE 5.8.3 `CaptiveSky_2Editor` compiled the basin code, and
`CaptiveSky2.Agent.IslandRainBasin` passed in a scratch editor run with agent
thinking disabled, model requests capped at zero, and a 60-second process cap.
The regression covers rain/sun accumulation bounds, once-per-day behavior,
leaf aging and capacity, state serialization/rejection, basin geometry values,
and resident-facing descriptions. Local log:
[`Codex_RainBasin_20261008.log`](../../Saved/CompileScratch/Codex_RavenCrabAttention_20261008/Saved/Logs/Codex_RainBasin_20261008.log).

This verifies deterministic rules, not the saved Island placement, observed
water response to weather over time, residents' willingness to interact, or
whether the basin and floating leaves read clearly at normal gameplay distance.
The connected editor currently has unsaved Rhododendron material work, so leave
that session alone; the next check is a short PIE observation after its owner
has preserved those edits.
