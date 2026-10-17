# Rainbow weather input guard — 2026-10-18

`ComputeRainbowStrength` previously clamped ordinary out-of-range weather
values but did not reject NaN or infinity. Those values can make the computed
strength non-finite and reach the transient rainbow material; the same value
could also be described to a resident as a faint bow. The dew calculation
already uses a finite-input guard for its environmental samples.

The rainbow calculation now returns zero if any of its five weather inputs is
non-finite, and `DescribeRainbow` returns no observation for a non-finite
strength. Regression cases cover each input field and the resident description.
Normal weather curves and the transient actor/material are unchanged.

## Verification

An isolated UE 5.8.3 `CaptiveSky_2Editor` target built successfully from
`Saved/CompileScratch/Codex_BasinRipple_20261018`, with
`-NoHotReloadFromIDE -MaxParallelActions=3`. The build compiled
`IslandRainbow.cpp` and `IslandRainbowTests.cpp`, then linked the editor module.
Log: `Saved/Logs/Codex_IslandRainbow_20261018.log`.

The focused `CaptiveSky2.Agent.IslandRainbow` automation test was not executed:
project-opening automation has a known pre-test Core null-access crash, and
this change does not alter that startup path. The build is compile/link
evidence only; runtime behavior and the visible rainbow remain unreviewed.
No main-project map or Content asset was opened or saved.

## Next step

Run the focused automation once project startup is independently healthy. No
visual review is needed for this defensive-only change; resume the higher-value
Megaplant and shoreline candidate review in a disposable Unreal destination
when the editor is responsive.
