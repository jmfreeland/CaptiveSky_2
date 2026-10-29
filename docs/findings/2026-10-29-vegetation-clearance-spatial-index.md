# Vegetation clearance spatial index — 2026-10-29

## Change

Runtime woodland placement previously tested each candidate against every
accepted tree, sapling, shrub, woodland flower, and coppice location. The
bounded Island population is in the thousands for these groups, so those
incremental full-list scans grow needlessly as scatter is built.

Added `FIslandSpatialPointIndex`, a deterministic 2D grid with 10 m cells.
It searches only cells overlapping a candidate's clearance radius, then uses
the same exact `FVector::Dist2D(Point, Existing) < Radius` check as the prior
loops. The tree, sapling, shrub, rhododendron, coppice, and meadow-flower
clearance queries now use the index. Species selection, random-stream order,
spacing thresholds, trace limits, and population budgets are unchanged.

## Verification and limits

- UE 5.8.3 `CaptiveSky_2Editor` compiled and linked successfully in 12.12 s
  (7 UBT actions), including `IslandWeather.cpp`, the index, and the new
  `IslandSpatialPointIndex` automation test.
- The automation compares 256 seeded queries against brute-force distance
  checks and covers negative coordinates, exact-radius boundaries, empty/reset
  behavior, and invalid coordinates. It compiled but was not run because the
  editor is currently unavailable and recent headless editor launches have
  repeatedly crashed before tests begin.
- No benchmark or Island runtime result is claimed. Placement-count parity,
  initialization time, frame rate, and memory impact remain to be measured in
  a working, request-disabled editor session. No map or Content asset changed.
