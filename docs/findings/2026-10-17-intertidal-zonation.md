# Local intertidal response for Tideglass periwinkles

The three periwinkles previously received one shared tide response regardless
of the height of their individual shore rocks. The new local waterline offset
is `global tide offset + pool sea-level Z - periwinkle home Z`. A snail lower
than the mean waterline therefore remains wetter during the same ebb, while a
snail higher on the shore tucks sooner. The existing rain, visitor, crab,
grazing and color-variant responses are unchanged. If the Tideglass pool
surface cannot be found, behavior falls back to the existing global tide
response.

This is intentionally lightweight: it adds no actor, component, trace, timer,
model request, or saved-world state. The static mesh lookup happens once at
BeginPlay; each snail keeps its original home elevation for later evaluations.

## Validation (2026-10-17)

- UE 5.8.3 scratch editor target built successfully with
  `-NoHotReloadFromIDE`; UBT compiled `IslandTidepoolPeriwinkle.cpp` and
  `IslandTidepoolPeriwinkleTests.cpp` and linked the editor module.
- Added `CaptiveSky2.Agent.TidepoolPeriwinkleIntertidalZonation` coverage for
  the mean-water fallback, higher and lower shore bands, relative tuck/wetness,
  invalid inputs, and extreme finite elevations.
- The focused NullRHI launch exited without creating its configured automation
  log, so this test has **not** been confirmed running. A contemporaneous crash
  bundle in the shared `Saved/Crashes/_0000` folder has the project's known
  Core startup signature, but its custom log names an unrelated Norway Maple
  preview command; it cannot be attributed to this test invocation. No
  behavioral pass or in-game visual claim is made.

Next: once project startup is stable, run the focused test and inspect one low
and one high shore snail through a complete Tideglass cycle. Confirm the actual
periwinkle elevations relative to the rendered pool surface before tuning the
response thresholds.
