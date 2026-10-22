# Broadleaf share follow-up (2026-10-22)

## Change

The earlier grove pass capped maple, aspen, and beech at 600 instances each,
with at most five broadleaf replacements per eligible grove. That spread the
species to 363 of 500 groves but left the wide landscape visually spruce-heavy.
The pass now allows up to ten broadleaf substitutions per mixed grove and
raises the per-species cap to 1,200. These are substitutions for spruce in the
existing 32-tree grove budget; they do not add trees to the landscape. The
longer-scale habitat field still assigns species by grove, preserving grouped
woodland character rather than alternating species tree-by-tree.

## Validation

- UE Live Coding compile succeeded (`UnrealEditor-CaptiveSky_2.patch_21`).
- A bounded Simulate pass generated 84,737 ground-cover instances and 14,430
  trees (8,745 saplings). It reported 1,200 each of Norway maple, European
  aspen, and European beech across 381/500 groves, with 39 birch and 57 hazel
  coppices. The overall tree population stayed unchanged.
- `CaptiveSky2.Agent.IslandWeather` passed in 0.065 seconds. Its transient-world
  fixture still emits the existing “No Island ground-cover instances placed”
  and context-free actor-destruction warnings; the test reported no failures.
- The same wide editor viewpoint remained visually sparse and did not make the
  individual broadleaf silhouettes obvious. The increased species count is
  verified, but close-range material and composition review remains open.
- No new texture-pool or RHI exhaustion line appeared in the sampled editor
  log. This is not evidence that the recurring graphics-memory warning is
  resolved: an earlier same-day Island capture recorded Unreal's “Video memory
  has been exhausted (179.7 MB over budget)” overlay, and the brief simulation
  did not sample peak GPU allocation. Keep full-length playtests deferred until
  VRAM headroom and in-view streaming stats are captured together.

## Next checks

1. Inspect maple, aspen, and beech in a close in-world view to confirm mesh and
   material identity, then choose a representative long and short view for
   composition review.
2. Capture `stat streaming` and GPU allocation in the same view/session while
   reproducing the memory overlay; do not raise the pool based only on its
   configured 4096 MiB capacity.
3. Validate player-input water entry, surface travel, and shore exit separately
   from the engine swimming-mode unit test; a swim pose is still not supplied by
   the current animation set.
