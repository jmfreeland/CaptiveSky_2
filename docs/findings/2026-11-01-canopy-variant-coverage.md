# Canopy species coverage versus visible variety — 2026-11-01

## Evidence already available

The bounded 120-second `-NullRHI` spatial diagnostic records mature-tree
placement, not pixels or player visibility. It found 5,614 mature trees across
117 occupied 200 m tiles: 2,014 spruce and 900 each of Norway maple, European
aspen, European beech, and black alder. Of those tiles, 111 (95%) contain at
least two mature species; 45 contain all five. The same startup recorded
24,808 broad-pass ground-cover placements across 143 200 m tiles, with only
one valid tile empty. These are useful distribution checks, but NullRHI cannot
establish whether canopies look distinct, whether the ground feels naturally
scattered, or whether the view performs acceptably.

The live project source now assigns a deterministic A/B canopy variant per
broadleaf tree while preserving the fixed species populations. This reduces
single-silhouette repetition without adding instances. The installed
Megaplant audit reports four A-D meshes per species, but C and D remain
unqualified for Island rendering, wind, Nanite cost, and memory. Existing
broadleaf cull distances are 80-125 m for maple, aspen, and beech, and
120-220 m for alder, compared with 550 m for spruce. Thus an island-wide
species census can be healthy while distant visitor silhouettes become
spruce-dominated; whether the A/B split is visibly distinct still needs a
rendered comparison.

## Next experiment

The A-only baseline is now an opt-in launch profile: `-IslandBroadleafAOnly`
forces requested B selections to A without changing the deterministic placement
field, tree populations, or cull distances. The normal launch remains seeded
A/B. A startup log identifies the selected profile and retains per-species
A/B instance counts. Once the whole-device GPU has at least 4 GiB free, capture
matched daylight woodland views with and without the flag using the same world
seed, route, and scalability. Record close/middle/far silhouette readability,
p95 frame time, and whole-device VRAM. Stop on a video-memory warning or
sustained frame rate below 30 FPS. Do not raise population or cull distances
unless this constant-population comparison shows headroom.

## Post-restart GPU gate recheck — 2026-11-01

After reopening the actual D: project, a follow-up `nvidia-smi` sample showed
9,600 / 12,282 MiB used (2,682 MiB free), 42% utilization, and 73 °C. This is
still below the 4 GiB rendered-session gate. The NVIDIA WDDM process table
reported no per-process entries, so this sample does not identify which
applications account for the reserved memory. No PIE or Game session was
started, and no new frame-time or visual evidence was collected.

Sources: `Saved/CS58/Saved/SessionReports/TreeSpatialNullRHI_20261031.md`,
`docs/findings/2026-10-31-nullrhi-tree-spatial-audit.md`, and
`docs/findings/2026-10-31-megaplant-tree-variant-audit.md`.

## Live source and GPU recheck — 2026-11-01

Read-only inspection of the open D: project's `IslandWeather` source confirms
that all four broadleaf species have A/B components and use a stable local
variant selector. The viewport currently shows a close woodland-floor path,
not a view that can establish far-canopy diversity. The whole-device GPU sample
was 8,898 / 12,282 MiB used (3,384 MiB free, 58% utilization, 78 C), below the
4 GiB rendered-session gate. No rendered comparison was started. Do not infer
that the variant code is missing from the live project or visually successful
from the source and placement census alone.

## A-only comparison profile — 2026-11-01

Added the explicit `-IslandBroadleafAOnly` profile for a matched A-only
appearance baseline. It resolves every requested broadleaf B variant to A
without consuming scatter RNG or changing tree counts and placement candidates;
the normal profile remains deterministic A/B. A focused automation regression
covers the resolver, and the UE 5.8.3 isolated Editor target compiled and linked
successfully (39 actions, followed by a 4-action incremental relink). The
automation has not run under Unreal, and the profile has not been rendered.
After the build, whole-device GPU memory was 8,898 / 12,282 MiB used (3,384 MiB
free), still below the 4 GiB gate; keep the visual comparison pending until
memory headroom is available.
