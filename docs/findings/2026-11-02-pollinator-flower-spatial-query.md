# Bound nearby flower searches for the dragonfly — 2026-11-02

## Change

`AIslandTideglassDragonfly` periodically asks `AIslandWeather` for the nearest
currently visible meadow flower within 9 m. The old query scanned every
instance of all 11 meadow-flower meshes on each check. The weather actor
already builds a 15 m 2D cell index for those flower instances to update local
sway, so the nearest-flower query now visits only cells overlapping its local
search radius and still applies the exact prior world-space distance, bloom,
visibility, and instance-scale checks.

The index is valid for ordinary initialized Island flowers because their
instance locations do not move when their seasonal bloom scale changes. If a
preview or test has not built the cache, the query preserves the former full
scan as a fallback. Large query radii and degenerate component scales also
fall back instead of expanding into an unbounded cell walk. No flower,
dragonfly, interaction, or model-request behavior changes.

## Validation and limits

- UE 5.8.3 `CaptiveSky_2Editor` compiled and linked in the isolated
  `Saved/CS58` project (20 UBT actions); both `IslandWeather.cpp` and
  `IslandEcologyTests.cpp` compiled.
- The seasonal-flower fixture now builds the same cell map and covers current
  bloom visibility, a query crossing a cell boundary, component translation
  into negative world coordinates, and the uncached-preview fallback.
- Automation was not run. The project's isolated editor startup has a
  recurring pre-test access violation; do not repeat its launch recipe without
  new crash evidence or a materially changed startup context.
- No runtime timing, frame-rate, or rendered gameplay result is claimed. The
  optimization is a bounded spatial lookup replacing a full population scan;
  measure it in a healthy runtime session when that startup gate clears.

This reduces unnecessary ecology-query work while preserving the existing
dragonfly visits that residents singled out as part of Tideglass feeling alive.
