# Dragonfly rain interruption — 2026-10-17

The Tideglass dragonfly already declined to begin a flower visit in heavier
rain, but an active eight-second hover could continue if a shower arrived
mid-visit. `CheckForNearbyMeadowFlower` now samples rain before its active-visit
early return and interrupts a hover above the same 0.45 intensity threshold.
The interruption is logged through the existing opt-in telemetry, clears only
the temporary interest, and leaves the 16-second visit cooldown intact. Since
the flower check is paced at 1.2 seconds, a newly heavy shower can end the
flower-directed flight at the next check; it does not add per-frame weather
work or affect flower instances.

`IslandEcologyTests.cpp` now covers the complete sequence: heavier rain
prevents a new visit, rain beginning during a visit interrupts it, the
remaining cooldown prevents immediate retry when the weather clears, and a
later visit can begin after that cooldown.

The isolated UE 5.8.3 editor target built successfully (5 actions, including
both changed translation units and module link). The focused NullRHI automation
has not run: project startup still crashes before test discovery with the
known Core null-access signature documented in
[`2026-10-17-project-open-crash-triage.md`](2026-10-17-project-open-crash-triage.md).
This verifies compilation/linking only, not runtime behavior or visual
readability. No map or Content asset was changed.
