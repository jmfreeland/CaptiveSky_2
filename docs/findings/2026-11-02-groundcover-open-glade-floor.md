# Soften bare ground in open meadow glades — 2026-11-02

## Evidence and change

The live Island editor viewport shows broad exposed soil between scattered
groundcover tufts. This is a local editor view, not a player-runtime or
island-wide coverage measurement. The existing broad-meadow acceptance field
deliberately leaves open glades, but its minimum acceptance was only 8%.

Raised the open-glade floor to 14% and reduced the nested patch strength from
3.16 to 2.65. The high-acceptance lush masses remain saturated at 100%, while
the lowest-acceptance habitat receives more candidates. The deterministic
128-by-128 field sample used by the focused automation predicts a 45.9% mean
acceptance (the existing target is 46% ±1%). Candidate-generation limits,
mesh selection, species fields, and the configured density control are
unchanged; this redistributes the same bounded broad-pass budget rather than
adding a vegetation layer.

## Validation and remaining gate

- UE 5.8.3 `CaptiveSky_2Editor` compiled and linked in the isolated
  `Saved/CS58` project. Both `IslandWeather.cpp` and
  `IslandWeatherBroadleafTests.cpp` compiled; five UBT actions succeeded.
- The focused `IslandWeather.MeadowHabitat` automation covers full lush
  coverage, the new 14% open-glade floor, high-to-low habitat contrast,
  invalid-input bounds, deterministic fields, and near-46% average acceptance.
  Automation was not executed because the current UE startup still exits with
  the previously recorded zero-second access violation.
- No rendered comparison, measured instance count, FPS, VRAM, or texture-pool
  result is claimed. The live editor is a separate D: project and has not
  loaded this source change. A deliberate player-height meadow/woodland route
  with frame-time and whole-device memory measurements remains necessary.

The bounded visual question is whether the 14% floor softens large bare patches
without making the meadow feel uniformly planted. If that trade-off is not
visible or costs too much, retain the existing seeded field and adjust from a
matched rendered capture rather than raising the total candidate ceiling.
