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

The current source resolves one A variant per broadleaf species. The installed
Megaplant audit reports four A-D meshes per species, but these additional
variants have not been qualified for Island rendering, wind, Nanite cost, or
memory. Existing broadleaf cull distances are 80-125 m for maple, aspen, and
beech, and 120-220 m for alder, compared with 550 m for spruce. Thus an
island-wide species census can be healthy while distant visitor silhouettes
become spruce-dominated; close-range repetition can likewise persist because
only one broadleaf silhouette is selected per species.

## Next experiment

Once the whole-device GPU has at least 4 GiB free, compare a matched daylight
woodland view with the current A-only baseline and a small A/B variant split.
Keep the total tree count, species counts, cull distances, camera, and
scalability fixed. Record close/middle/far silhouette readability, p95 frame
time, and whole-device VRAM. Stop on a video-memory warning or sustained frame
rate below 30 FPS. Do not raise population or cull distances unless this
constant-population comparison shows headroom. The last observed device sample
was only 2.4 GiB free, so no rendered comparison was started.

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
