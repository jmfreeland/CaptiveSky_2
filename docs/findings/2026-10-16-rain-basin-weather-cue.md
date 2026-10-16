# Rain basin weather cue — 2026-10-16

The persistent rain basin now gives rainfall one small local consequence: when
its water is deep enough, a restrained expanding ring appears at a randomized
point on the surface. Impacts are separated by 1.5–3.6 seconds according to
rain intensity and jittered so they do not become a metronome. The ring is a
single 40-segment procedural mesh, has no collision or shadow, and only enables
its actor tick for its 1.1-second life. It is cosmetic and is not added to the
saved basin record; light rain and a too-dry basin remain visually quiet.

The focused source changes compiled and linked successfully in the isolated
UE 5.8.3 `CaptiveSky_2Editor` target (11 build actions), including
`IslandRainBasin.cpp` and `IslandRainBasinTests.cpp`. The automation covers
the bounded radius/pulse curve, the rain-rate threshold and spacing, invalid
rain input, transient activation, expiry, and reuse. The Unreal automation was
not executed: project-opening launches are currently failing before project
modules load with the documented zero-second Core access violation. No editor
or saved Island state was modified, and no model requests were made.

Next validation: once the startup crash is symbolized or the environment
meaningfully changes, run `CaptiveSky2.Agent.IslandRainBasin` with thinking
disabled, then capture one matched wet/dry basin view in Game. Keep the feature
only if the ring reads softly on the shallow surface and remains unobtrusive.
