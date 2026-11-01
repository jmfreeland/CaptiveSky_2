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
The post-scatter startup report also logs A/B instance counts for each species,
making it possible to distinguish a valid-land sampling imbalance from an asset
or DynamicWind fallback in the next runtime check.

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
- A dedicated `CaptiveSky2.Agent.IslandWeather.BroadleafNaniteReadinessAssets`
  regression loads all eight A/B skeletal meshes and checks both
  `IsNaniteEnabled()` and `IsNaniteAssembly()`. It compiles and links in the
  isolated UE 5.8.3 target. This distinguishes a project-level Nanite Foliage
  setting from per-asset Nanite/assembly readiness, the three-part foliage
  pipeline described in Epic's [Nanite Foliage](https://dev.epicgames.com/documentation/en-us/unreal-engine/nanite-foliage)
  and [Nanite Assemblies](https://dev.epicgames.com/documentation/unreal-engine/nanite-assemblies?lang=en-US) documentation.

## Isolated runtime validation — 2026-11-01

The isolated UE 5.8.3 scratch project now runs `NullRHI` automation successfully.
`CaptiveSky2.Agent.IslandWeather.Broadleaf` discovered and passed all four
focused tests: A/B assignment and fixed tree budget, broadleaf habitat,
species-height profiles, and Nanite readiness for all eight A/B skeletal meshes.
The readiness test confirmed every loaded mesh is both Nanite-enabled and a
Nanite Assembly. This run also exercised asset loading and Nanite assembly
building in the scratch process; it did not render the Island.

`CaptiveSky2.Agent.InstancedSkinnedFoliageProvider` also passed. Its installed
Megaplant checks load the A/B tree assets, validate enabled DynamicWind skeletal
profiles, and verify that the instanced-skinned component selects UE's
DynamicWind transform provider. The project-local asset preflight found all
eight expected A/B packages in the scratch project's read-only Content link.

A bounded 120-second NullRHI Game audit then loaded the Island with the rebuilt
worktree module, disabled agent thinking, and a zero-model-request cap. It
reached the normal landscape scatter and ended automatically after 120.2 real
seconds with zero model requests and no automatic restart. Startup recorded
15,513 trees, 8,537 saplings, 15,555 broadleaf shrubs, and 1,489 rhododendrons
across 500 groves; broadleaf trees remained at the fixed 3,600-instance budget,
with all four species at 900 and represented across 372 groves. The A/B counts
were Norway maple 480/420, European aspen 429/471, European beech 464/436, and
black alder 439/461. This confirms actual instance placement uses both
silhouettes without increasing the mature broadleaf population. NullRHI still
provides no rendered appearance, frame-time, or VRAM comparison.

The actual D: `CaptiveSky_2` editor was restarted on 2026-11-01 and its fresh
project log records `r.Nanite.Foliage:1`, but it still has a separate module;
these tests validate the worktree source and linked assets, not that the live
Island is currently using the A/B canopy.

A fresh whole-device sample after the restart reports 9,606 / 12,282 MiB used
(2,392 MiB free), 46% utilization, and 71 °C. This remains below the project's
4 GiB rendered-session gate. Do not start PIE/Game just to inspect the split at
this headroom; WDDM's device-wide figure cannot attribute memory to Unreal
alone. The live project's Nanite setting and this worktree's new canopy remain
separate validation items.

Next, after the worktree build is loaded into a validation editor and device
headroom allows a rendered session, compare the same daylight woodland view
against the A-only baseline. Keep camera, total tree count, species counts,
cull distances, and scalability fixed. Record close/middle/far silhouette
readability, frame-time p95, and whole-device VRAM; retain the split only if it
improves visible variety while remaining above 30 FPS and below the memory
warning threshold. The last recorded free-VRAM sample was about 2.4 GiB, below
the project's 4 GiB PIE gate; no rendered comparison or long resident play run
was started.
