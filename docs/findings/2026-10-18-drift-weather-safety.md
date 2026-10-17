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
target in an isolated, minimal scratch project (175/175 UBT actions). This
verifies compilation, not the main project's full plugin/content startup.

The focused `CaptiveSky2.Agent.IslandDrift` commandlet attempt produced no log
or automation result, so it is **not counted as a test pass**. Do not infer a
result from its shell exit alone. Once a usable Unreal test session is available,
run the focused test, then capture one ordinary breeze and one rain-cleared
view to confirm the unchanged visual response.
