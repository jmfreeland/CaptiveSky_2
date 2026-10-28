# Meadow spatial diagnostics

The optional `CaptiveSky.Island.GroundCoverSpatialDiagnostics=1` path already
measured eligible landscape samples and placements on a 200 m grid, but its log
reported only island-wide totals and percentiles. Those aggregates could not
identify where sparse cover occurred.

When the diagnostic is enabled, `AIslandWeather::InitializeGroundCover` now
emits one deterministic log row per populated tile, ordered by tile Y and then
X. Each row includes the tile coordinate, eligible terrain-cell count,
broad-meadow-pass placement count, and placements per eligible cell. Coordinates
are floor-divided from world X/Y by 20,000 cm, so tile `[0,0]` covers world
coordinates `[0, 20,000)` on both axes. The denominator counts sampled cells
whose trace hit a landscape, passed the existing grade test, and cleared the
sea-level threshold; it is a sample count, not an exact area measurement.

“Broad-pass placements” counts only instances successfully added by the broad
landscape meadow pass. It does not count tree groves, shoreline or anchor
patches, or every other vegetation system. Tiles with zero accepted terrain
samples are absent because the scatter pass does not measure them. The CVar
remains off by default, so this adds no normal-session log volume or scatter
work beyond the existing disabled-branch check.

## Validation status

UE 5.8 Live Coding compiled and linked successfully. The focused
`CaptiveSky2.Agent.IslandWeather` automation test passed. Its fixture emitted
the existing no-landscape scatter and temporary-world cleanup warnings; it
does not exercise the tile-row output. The CVar was not enabled and no Island
regeneration was run, preserving GPU headroom. Enable it for one bounded,
request-disabled regeneration only when memory headroom is acceptable, then
compare the tile rows with daylight visitor-view captures before tuning scatter
density or fields.
