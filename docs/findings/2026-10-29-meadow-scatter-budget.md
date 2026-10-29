# Meadow scatter budget split — 2026-10-29

## Evidence and diagnosis

The latest available 5%-density diagnostics are from
`Saved/Logs/CaptiveSky_2-backup-2026.10.28-21.08.46.log`:

- The total candidate cap was 96,000 (5% of 1,920,000).
- 84,762 ground-cover instances were placed.
- The broad landscape pass accounted for 8,171 placements across 143 200 m
  tiles, while the remaining placements came from landmark-centered meadow
  patches. The terrain mask had 8,317 valid cells, with only two valid tiles
  receiving no broad-pass plants.

The broad pass was spatially spread, but received only about 9.6% of the final
placements because each accepted landmark patch consumed 1,875 candidates
before the broad-landscape budget was calculated. This is a plausible cause of
the repeated visual report that vegetation feels concentrated near places
rather than belonging to the landscape between them.

## Change

Keep the 1,920,000 base candidate ceiling and current density control. Reserve
at most 40% of the scaled candidate capacity for medium-distance
landmark-centered patches, capped at 12 patches per landmark. Seed-shuffle the
anchor order and visit it round-robin so Outliner order does not determine which
landmarks receive the scarce patch slots. Any capacity not used by successful
patches goes to the broad terrain pass. The existing close landmark verge
scatter and all plant-instance counts remain subject to the same hard cap.

At 5% density, the split allows at most 20 of the 1,875-candidate patches
(37,500 candidates), leaving at least 58,500 candidates for the broad terrain
pass. At higher density, the same share and total cap apply. This predicts a
more continuous mid-distance habitat and fewer repeated local rings without
increasing the configured candidate ceiling; visual quality and real runtime
cost still require a matched Game review.

## Verification

The isolated UE 5.8 scratch target built and linked successfully (37 actions,
120.31 seconds), including the changed `IslandWeather.cpp` and
`IslandMeadowBudgetTests.cpp`. The headless test
`CaptiveSky2.Agent.MeadowBudgetSplit` passed (exit code 0). The scratch copy is
based on the 2026-10-23 compile snapshot with this change applied; this is not a
build of the current full working tree. A bounded build of the current project
then passed UHT and compiled `IslandWeather.cpp`, `IslandWeather.h` consumers,
and `IslandMeadowBudgetTests.cpp`; final linking failed because
`UnrealEditor-CaptiveSky_2.dll` was in use by `UnrealEditor.exe`. The editor was
left untouched. `git diff --check` passed. No controlled Game/PIE validation or
map/Content edit is included in this change. Recheck the same
Tideglass-to-Woodland view with spatial diagnostics enabled; compare broad-pass
placements, tile coverage, total instances, p95 frame time, and peak GPU memory
against the 2026-10-28 sample before tuning the 40% share.
