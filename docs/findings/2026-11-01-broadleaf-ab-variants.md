# Broadleaf A/B variant support — 2026-11-01

## Change

`AIslandWeather` now loads the installed B skeletal mesh for each currently
scattered broadleaf species: Norway maple, European aspen, European beech, and
black alder. Each species keeps its existing A component and gets a paired B
`UInstancedSkinnedMeshComponent`. A separate, smoothed, seeded local field
assigns each valid broadleaf tree to A or B; it does not consume the existing
placement random stream, so tree locations, species choice, and species-specific
height sampling remain stable. If a B mesh, its bounds, or its enabled
DynamicWind profile is unavailable, that tree falls back to A.

This keeps the mature-tree population and broadleaf cap unchanged (3,600 total,
with the same per-species limits), retains each species' current cull distances,
and preserves non-collision/non-navigation behavior. Both variants are configured
with the existing instanced-skinned DynamicWind setup. B is only selected when
its asset has enabled DynamicWind data, avoiding static, windless replacements.
Nearby-plant perception already enumerates all instanced-skinned components, so
visible B instances use the same observation path as A.

## Validation completed

- UE 5.8.3 `CaptiveSky_2Editor` scratch target compiled and linked successfully
  with `-NoHotReloadFromIDE`; outputs remained in `Saved/CS58/`, separate from
  the open D: editor's DLL.
- The project-local tree asset preflight checked all eight A/B package paths
  against the worktree-linked `Saved/CS58/Content/` junction: 8 found, 0 missing.
- The complete AssetPipeline helper suite passed: 112 tests, using Blender
  5.2.2's bundled Python 3.13.13.
- The focused Unreal regressions now check deterministic A/B assignment,
  representative Island-scale use of both variants, unchanged Black Alder B
  culling/collision/navigation properties, asset presence, and DynamicWind
  provider support for all four B meshes.

## Not yet verified

The focused Unreal automation has not run against this build, and the live
editor still holds its previous DLL. A current `nvidia-smi` sample reports
9,489 / 12,282 MiB used (2,509 MiB free), below the project's 4 GiB rendered
session gate. Do not start PIE/Game just to inspect the split at this headroom.
The established NullRHI startup crash also remains unresolved, so no identical
headless retry was made.

Next, after the worktree build is loaded into a validation editor and device
headroom allows a rendered session, compare the same daylight woodland view
against the A-only baseline. Keep camera, total tree count, species counts,
cull distances, and scalability fixed. Record close/middle/far silhouette
readability, frame-time p95, and whole-device VRAM; retain the split only if it
improves visible variety while remaining above 30 FPS and below the memory
warning threshold.
