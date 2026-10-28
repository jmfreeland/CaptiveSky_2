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
does not exercise the tile-row output. A live Island regeneration with the CVar
enabled produced this summary:

```text
Landscape meadow scatter sampled 8317 valid terrain cells and placed 84762
ground-cover instances after 136001 bounded traces (budget 96000, density 0.05).
Meadow spatial diagnostics: 200 m tiles=143, valid cells=8317,
broad-pass placements=8171, zero-cover tiles=2 (1.4%),
placements/valid-cell mean=0.98 p10=0.71 p50=0.98 p90=1.20 max=4.00.
```

The per-tile rows show broad-pass coverage is generally close to one placement
per eligible sampled cell; two sampled tiles had no broad-pass placements. The
normalization matters because a tile's eligible sample count is not uniform.
These counts exclude grove trees, shoreline and anchor patches, and other
vegetation systems. The run also logged 1,200 instances each of Norway maple,
European aspen, and European beech, plus 39 birch coppices and 57 hazel coppices;
the broadleaf trees spanned 381 of 500 groves. A night preview showed mixed
herbaceous cover and multiple tree silhouettes, but no daylight visitor-view
capture was made, so visual naturalness remains unverified.

### Safety-cap correction

The bounded-run setup was not applied as intended. `DefaultGame.ini` was edited
after Unreal had initialized the session subsystem, so PIE retained its cached
defaults of 1,800 seconds and 120 model requests rather than the temporary
15-second/zero-request cap. The run lasted 50.3 seconds and made five model
requests; it was manually stopped after host-wide GPU use rose to 11,639 MiB of
12,282 MiB. It stopped with zero requests in flight. Host-wide GPU use later
fell to 8,587 MiB. Process-specific VRAM attribution was unavailable, and the
log search found no video-memory-exhausted warning. The test therefore did not
meet the planned request or duration limits, even though it ended without a
logged exhaustion warning.

The on-disk session defaults were restored to 1,800 seconds/120 requests, the
diagnostic CVar was returned to `0`, and the editor was restarted. On this fresh
startup the project log confirms `bEnableNaniteFoliage:1` and
`r.Nanite.Foliage:1`. This confirms the setting is active, not that Nanite
Foliage improves appearance, VRAM, or frame time; those still need a controlled
visual/performance comparison.

Before another bounded PIE, set the cap before launching Unreal, restart the
editor, and verify the startup log explicitly says `15 seconds, at most 0 model
requests` before allowing the world to play. Then compare daylight visitor-view
captures with the per-tile rows before tuning scatter density or fields.
