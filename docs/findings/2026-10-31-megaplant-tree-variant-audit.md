# Megaplant tree-variant audit

The active D: project contains Megaplant assets for Norway maple, European aspen, European beech, and black alder. Each species has four exported forest meshes, A–D. The runtime source currently loads only variant A for each species, so all four species can be present while individual silhouettes still repeat. The current broadleaf cull ranges are 80–125 m for maple/aspen/beech and 120–220 m for black alder; the spruce persists to 550 m. This makes a distant view especially likely to collapse to spruce-only silhouettes.

The asset audit was read-only. The source and build used for the preceding trail-viewpoint commit are in the Codex worktree; the open D: project is separate, and its content is not synchronized from that branch.

Next experiment: after the editor opens, capture a fixed woodland view and record per-species instance counts, GPU memory, and frame-time p95. Then compare a constant-population variant split (start with A/B, not all four) against the current A-only baseline. Keep cull distances unchanged during that comparison. Only consider extending far silhouettes if the matched run remains below the existing GPU-memory gate and above the 30-FPS p95 floor.
