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

`CaptiveSky2.Agent.ResidentWanderPaths` checks worn-versus-untrodden route
affinity, degenerate paths, and the strict 3% score bound. Its first runtime
attempt exposed a fixture mismatch: it seeded 80 steps per route cell while
asserting affinity above 0.9, but the wear curve reaches full wear at 160 steps.
The route-scoring code was not changed; the fixture now seeds the declared
`WearFullSteps` threshold.

The isolated UE 5.8.3 editor target rebuilt and linked the corrected test, and
`ResidentWanderPaths` completed with `Result={Success}` and exit code 0. The run
used `-NullRHI -NoSound -DisablePython`, disabled agent thinking, set the model
request cap to zero and the realtime cap to 60 seconds, and wrote only to the
scratch world-data root. The interactive editor and its unsaved Island level
were untouched. Log:
[`ResidentWanderPaths_FullWear.log`](../../Saved/CompileScratch/Codex_TidalCrabActivity_20261008/Project/Saved/Logs/ResidentWanderPaths_FullWear.log).

This proves the deterministic route-affinity and tie-break bounds, not whether
residents choose to wander, how often trails become worn in ordinary play, or
whether the ground marks read clearly in a rendered Game/PIE view. Compare
bounded movement logs with empty and populated trail data during a future
in-world pass.
