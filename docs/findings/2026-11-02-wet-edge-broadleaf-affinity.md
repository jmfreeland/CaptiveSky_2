# Wet-edge broadleaf affinity — 2026-11-02

## Change

The seeded five-species canopy field now uses the existing Tideglass wet-edge
moisture signal to favor Black Alder in a patchy band extending up to 50 m
beyond the pool's measured foliage-clearance edge. A separate 26 m smooth
field keeps the preference spatially coherent; the other four tree species
share the remaining dry-field selection. Missing Tideglass geometry and
invalid moisture fall back to the original field.

Tree population/species caps, mesh variants, transforms, collision, navigation,
and per-frame work are unchanged. The feature adds no foliage instances; it
only changes which existing species occupies an eligible tree position.

## Validation and limits

The isolated UE 5.8.3 `CaptiveSky_2` Game target built and linked successfully
in `Saved/CS58`; both `IslandWeather.cpp` and
`IslandWeatherBroadleafTests.cpp` compiled. The focused test now checks that
saturated wet habitat favors Alder, retains a mixed canopy, stays deterministic,
clamps moisture, and falls back safely for invalid values. Its Editor automation
has not run: the open D: editor has a different module, and prior isolated
NullRHI startups crashed before test discovery. No rendered appearance,
performance, or GPU-memory claim is made.

Next: run `CaptiveSky2.Agent.IslandWeather.BroadleafHabitatSelection` in a
stable automation session, then compare the same Tideglass-to-woodland Game
view and check the 30-FPS floor and whole-device VRAM. Keep the affinity only
if Alder reads as a wet-edge community without creating a single-species ring.
