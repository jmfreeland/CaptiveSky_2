# Trail alignment awareness

The existing trail ledger summarizes worn ground in bounded 12 m regions and deliberately avoids inventing continuous routes. Residents could report that a region is worn, but could not say whether the patches themselves form a clear line.

The region summary now derives a weighted principal axis from existing worn cells when at least four contribute. A compass alignment is exposed only when covariance anisotropy is at least 0.45; horizontal, vertical, and diagonal lines receive undirected compass labels. Sparse or isotropic clusters (including a cross) produce no alignment claim. This is a transient derived summary: no save format, ledger data, map, scatter, or model-call behavior changes. It does not imply direction of travel or a continuous path.

Validation: the UE 5.8.3 scratch editor target compiled and linked successfully, including `IslandTrail.cpp` and `IslandTrailTests.cpp` (25 actions). A separate earlier `ResidentWanderPaths` automation log is not evidence for these new tests. The attempted focused `IslandTrail` automation launch produced no corresponding log/result, so runtime assertions and in-editor behavior remain unverified.
