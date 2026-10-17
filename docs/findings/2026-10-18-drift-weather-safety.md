# Windborne drift weather safety — 2026-10-18

`AIslandDrift` now fails closed when its timestep, viewer position, wind, rain,
or daylight sample is non-finite. Invalid wind resolves to calm air, an invalid
viewer sample keeps the actor at its last valid position, and invalid rain or
daylight suppresses new motes. Extreme finite vector components are bounded
before speed and interpolation math. An already-active mote with corrupted
position, velocity, spin, or lifetime state is reset before it can upload an
invalid instance transform. Normal Island samples retain the existing
wind/rain/daylight behavior and the 96-mote cap.

`IslandDriftTests.cpp` adds coverage for malformed scalar and vector inputs,
including a runtime call that checks the actor, mote positions, and velocities
remain finite. The copied Source tree compiled and linked as a UE 5.8.3 editor
target in an isolated, minimal scratch project (175/175 UBT actions). The
scratch copies of both changed files were SHA-256-identical to the committed
sources.

The focused `CaptiveSky2.Agent.IslandDrift` automation then passed in that
minimal project under NullRHI: the log reports one matching test and
`Test Completed. Result={Success}`. It ran with agent thinking disabled,
`CaptiveSkyMaxModelRequests=0`, and scratch-local DDC and shader directories.
Log: `Saved/CompileScratch/Codex_DriftSafety_20261018/IslandDriftTest.log`.
This verifies deterministic behavior without the main project's Content; it
does not validate the live Island's visuals, ordinary gameplay readability, or
the full project's startup path. A real-RHI breeze and rain-cleared capture is
still the visual follow-up when the editor is usable.
