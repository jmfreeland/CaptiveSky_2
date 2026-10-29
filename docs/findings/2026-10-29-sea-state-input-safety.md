# Sea-state input safety — 2026-10-29

## Change

`UIslandSeaStateSubsystem` now sanitizes weather inputs before deriving the ocean's target sea state. Non-finite storm or wind samples are treated as calm. The smoothed transition keeps its current state in `[0, 1]`, holds that state for a non-finite or non-positive timestep, and preserves the current state if the target is invalid. Amplitude and steepness helpers also resolve non-finite inputs to their calm scales.

This prevents a malformed environment sample or timestep from propagating NaNs into the live Gerstner wave generator and its recomputed GPU data. Normal finite inputs preserve the existing storm response and rise/fall timing.

## Verification

- UE 5.8.3 `CaptiveSky_2Editor` target compiled and linked successfully on 2026-10-29 in 13.49 seconds (7 UBT actions), including `IslandSeaStateSubsystem.cpp` and `IslandSeaStateTests.cpp`.
- Added automation assertions for non-finite storm/wind/current/target/timestep inputs and bounded sea state and wave scales.
- The automation was not executed: the editor window is not targetable in the current desktop session, and the project's recent command-line Unreal launches have crashed during engine startup before tests begin. Compilation is not evidence that the test passed.

## Follow-up

Run `CaptiveSky2.Agent.IslandSeaState` in a working editor or after the pre-initialization engine crash has a new diagnostic. This change does not claim to address Nanite foliage performance or the editor startup crash.
