# Megaplant tree-variant audit

The active D: project contains Megaplant assets for Norway maple, European aspen, European beech, and black alder. Each species has four exported forest meshes, A–D. The runtime source currently loads only variant A for each species, so all four species can be present while individual silhouettes still repeat. The current broadleaf cull ranges are 80–125 m for maple/aspen/beech and 120–220 m for black alder; the spruce persists to 550 m. This makes a distant view especially likely to collapse to spruce-only silhouettes.

The asset audit was read-only. The source and build used for the preceding trail-viewpoint commit are in the Codex worktree; the open D: project is separate, and its content is not synchronized from that branch.

Next experiment: after the editor opens, capture a fixed woodland view and record per-species instance counts, GPU memory, and frame-time p95. Then compare a constant-population variant split (start with A/B, not all four) against the current A-only baseline. Keep cull distances unchanged during that comparison. Only consider extending far silhouettes if the matched run remains below the existing GPU-memory gate and above the 30-FPS p95 floor.

## Worktree status update — 2026-11-02

The current worktree has since implemented the first constant-population A/B
split for Norway maple, European aspen, European beech, and black alder. A
seeded 18 m spatial field chooses each tree's silhouette, the total mature-tree
budget and cull ranges stay fixed, and the normal profile now uses A/B; the
`-IslandBroadleafAOnly` command-line flag retains an A-only comparison baseline.
Common Hazel remains A-only. This implementation is in the C: worktree and is
not evidence that the separate live D: editor checkout has received it or that
the variants are visually distinguishable in the Island.

The focused asset-readiness automation already verifies the eight A/B assets
load and are Nanite assemblies. It now also verifies that all four B variants
retain enabled Dynamic Wind metadata, which is a production selection
requirement: without it, runtime deliberately falls back to variant A. Once
the headroom gate permits a rendered session, compare seeded A/B against
`-IslandBroadleafAOnly` at the same woodland view, keeping population and cull
ranges constant; record per-component counts, GPU memory, and frame-time p95
before deciding whether C/D or a new species adds useful value.

### Verification — 2026-11-02

`CaptiveSky2.Agent.IslandWeather.BroadleafNaniteReadinessAssets` passed in the
isolated UE 5.8 scratch project (`Saved/CS58/Saved/Logs/Codex_Broadleaf_DynamicWind_20261102.log`, exit code 0). This validates the asset loading, Nanite assembly, and enabled Dynamic Wind user-data checks for all four B meshes against the scratch content set. It does not validate the live Island's rendered distribution or GPU cost. During the run, Unreal logged repeated async skinned-asset compile estimates of 4.6 GiB against an available estimate that fell from about 4.1 to 3.9 GiB; keep the render/memory gate closed until a controlled in-editor check shows safe headroom.
