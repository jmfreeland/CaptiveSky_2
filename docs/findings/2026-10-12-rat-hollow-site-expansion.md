# Rat-hollow North Woodland site expansion — 2026-10-12

## Bounded read-only scan

The first human-height site at `(-104500, 105000)` was rejected after its paired
real-RHI review: the hollow opening could be read at true Rat eye height, but
the approach still looked like an exposed, repetitive slope. A NullRHI Game
scan then sampled four overlapping North Woodland patches to find alternatives.
It loaded `/Game/Maps/Island`, initialized the existing ground-cover scatter
(1,779,869 instances), and ran four `Island.RatHollowSiteAudit` commands. The
60.1-second session disabled resident thinking, allowed zero model requests,
used a separate world-data root, and exited normally. The audit left the saved
Island and canonical Rat home record unchanged.

Each patch tested 29 positions at 400 cm spacing with the existing audit's
ground slope, Rat-sized capsule/approach obstruction, NavMesh, local-understory,
and five-point spruce-crown checks. **All 116 positions were rejected only by
the strict crown-cover gate:** no ground, slope, or capsule/approach rejection
was reported, and every candidate recorded `crownSamples=0/5`. This is local
North Woodland evidence, not an Island-wide search. It shows the current
north-woodland tree arrangement does not satisfy the audit's definition of a
fully overhead-sheltered Rat home; it does not prove that a visually concealed
site is impossible.

The strongest visual-review near-miss is `(-105200, 104200, 2334)` cm: slope
range 9.1 cm, nearby understory in 5/5 footprint samples, NavMesh projection
in 5/5 footprint samples, all 4 cardinal approaches navigable and capsule-clear,
and the nearest mature spruce 312 cm away. A second accessible patch at
`(-103800, 104200, 2375)` cm has 5/5 understory and navigation samples, 4/4
clear approaches, and its nearest mature spruce 1,420 cm away. Neither is an
approved home: the headless scan cannot judge whether the prop disappears at
human height, whether its entrance reads from true Rat eye height, or whether
the final rotated/placed hollow remains capsule-clear.

## Decision and next step

Do not weaken the canopy requirement just to produce a passing result, and do
not add a canonical `places.json` entry. Next, compare the two accessible
near-misses in a disposable visual trial using true Rat-eye and human-approach
views. If neither gives convincing concealment with nearby spruce and existing
understory, the landscape needs a deliberate tall, irregular woodland-edge
composition (or usable exported broadleaf vegetation) before a Rat home can
read naturally; adding a hollow to another bare slope would repeat the rejected
result.

The visual editor launch did not reach world-ready within its 240-second guard,
so no new RHI screenshot or placement test was produced. The headless scan did
complete and can be reproduced from its log:
`Saved/Logs/Codex_RatHollowWideSiteScanNullRHI_20261012.log`.
