# Mature-tree spatial diagnostics — 2026-10-31

The Island's existing opt-in ground-cover spatial diagnostic reported broad
meadow placements by 200 m tile, but mature trees were only logged as
island-wide species totals. That could confirm a four-species population while
missing a locally monochrome visitor view. The same startup diagnostic now
also reports accepted mature-tree placements per 200 m tile, split into spruce,
Norway maple, European aspen, European beech, and black alder.

The report is observational only: it does not alter placement, species choice,
the 3,600-tree broadleaf budget, culling, or any runtime defaults. It excludes
saplings and understory so the counts answer the canopy question directly.
Tile coordinates are deterministic world-origin grid coordinates; output is
sorted before logging. With diagnostics disabled, the per-tile map remains
empty and recording returns immediately.

## Verification and use

- UE 5.8 editor target compiled successfully after this change (39.99 s).
- `git diff --check` passed for the source change.
- The diagnostic was not run against the live Island: the active D: project is
  separate from this source worktree and has not loaded this compiled module.
- Runtime tile counts, naturalness, FPS, VRAM, and visual tree readability
  therefore remain unverified.

On a rebuilt project, enable the existing diagnostic *before* Island scatter
using `-CaptiveSky.Island.GroundCoverSpatialDiagnostics=1`. The log will include
lines such as:

```text
Mature tree tile [-2,3] (200 m): spruce=14, Norway maple=3, European aspen=7, European beech=0, black alder=2.
```

Use those counts with a daylight Tideglass-to-woodland route and matched
frame-time/whole-device-memory observations. If a visible tree species is
missing from nearby tiles despite healthy island totals, adjust habitat
placement or distance readability—not the global vegetation budget—based on
the route evidence.
