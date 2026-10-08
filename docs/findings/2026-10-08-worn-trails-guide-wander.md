# Worn paths gently guide resident wandering (2026-10-08)

Explicit resident wandering now samples each complete candidate navigation route
against the persistent ground-trail ledger. A path's affinity is the average
wear of up to 256 evenly spaced samples, using the same wear curve as visible
ground marks. Routes with more established foot traffic receive at most a 3%
multiplier on their existing curiosity score. They do not create curiosity by
themselves, override a visible landmark's pull, alter a destination, or turn a
partial/blocked route into a valid move. With no trail subsystem or no worn
ground, candidate ranking is unchanged.

This connects a long-lived landscape trace to movement without adding a model
request, a new persistent record, or a scheduled action. Residents may still
choose whether to wander; the preference only shapes the route after they have
already chosen it.

`CaptiveSky2.Agent.ResidentWanderPaths` now checks worn-versus-untrodden route
affinity, degenerate paths, and the strict 3% score bound. The isolated UE
5.8.3 editor target compiled and linked successfully (146 actions, including
both edited translation units). Runtime automation is still unverified: the
first headless launch produced a scratch-only Unreal assert because its
user-level Zen/DDC graph had no writable node; a retry with the documented
memory-cache flags remained CPU-active for over six minutes without opening an
editor log and was stopped. The crash report identifies DDC configuration,
not this code. No user map, persistent Island state, or model request was
involved. Continue with a known-writable UE cache or a warm validation project,
then check `CaptiveSky2.Agent.ResidentWanderPaths`. This still would not prove
whether residents choose to wander, how often trails become worn, or whether
the marks read clearly in a normal Game/PIE view; compare bounded movement logs
with empty and populated trail data when the automation launch is reliable.
