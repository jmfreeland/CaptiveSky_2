# Validate persisted rain-basin offerings (2026-10-18)

The rain basin is a small weather-responsive landmark where residents can leave
one leaf afloat per day. Its water level and the bounded leaf record persist
between sessions. The loader was casting schema, Island-day, and random-seed
JSON numbers straight to `int32`; fractional, non-finite, or out-of-range leaf
values could therefore turn into misleading author/day/placement history.

`FIslandBasinState::FromJson` now requires an exact finite schema version and
validates each offering's day and seed before conversion. Day must be a
non-negative integral `int32`; seed must be an integral `int32`. Bad offering
records are skipped without losing otherwise readable water, location, or
other valid offerings. The eight-leaf cap is applied as soon as eight valid
records are recovered. Invalid schema data is rejected before replacing the
current state, while the existing fail-soft behavior for a malformed basin
location is preserved.

`CaptiveSky2.Agent.IslandRainBasin` now covers fractional schema versions
(including preserving existing state), fractional days, an overflowing seed,
and recovery of a valid neighboring offering. The existing tests for water,
wind-driven floating leaves, resident interaction, persistence, and
malformed-location recovery remain unchanged.

UE 5.8.3 compiled and linked both `IslandRainBasin.cpp` and
`IslandRainBasinTests.cpp` in the isolated scratch project. Runtime automation
was not run: the user's UnrealEditor remains present in the Windows app
inventory, but window activation times out, and the documented project-open
startup fault occurs before automation begins (see
[startup triage](2026-10-13-unreal-startup-triage.md)). No map, Content asset,
resident request, or live world-state file was modified.

Next verification: run only `CaptiveSky2.Agent.IslandRainBasin` once the
existing editor session is closed or responsive and the project can reach its
automation tests; keep agent thinking and model requests disabled.
