# Native spruce mesh review (2026-10-13)

The project already contains a native static mesh at
`/Game/PN_interactiveSpruceForest/Meshes/half/high/spruce_half_01`. I reviewed
it as a possible low-cost conifer addition before generating or placing more
vegetation.

## Evidence

UE 5.8.3 loaded the asset and completed real-RHI scene captures with a transient
preview actor beside `WindArch`. The Island map and source asset were not saved
or modified. The mesh reports three LOD0 sections and four assigned material
slots: trunk, branch, leaf, and imposter. Its bounds are approximately
431 × 367 × 1,674 cm. The two scene captures are local, gitignored artifacts:

- [Single mesh](../../Saved/Playtests/Codex_SpruceAssetReview_20261013/001_WindArch.png)
- [Two copies, second rotated 180°](../../Saved/Playtests/Codex_SpruceAssetReview_20261013/002_WindArch_Paired.png)

The single mesh reads as a sparse, narrow silhouette with a very dark crown in
the current Island lighting. Pairing a second copy at 180° did not make the
silhouette meaningfully fuller. The review script could not inspect the base
color graph because these are material instances, but all four slots remained
assigned and the mesh rendered without an import error.

## Decision

Do not scatter or promote this asset into the authored Island yet. Its
`half/high` source path and imposter slot suggest it may be a component of a
larger forest assembly rather than a finished 360-degree tree; the available
capture does not prove how it should be assembled or whether its foliage reads
well at gameplay distance. Find the intended full-tree assembly or another
complete native species, then review it under a known daylight capture and
measure a small test group against the established viewpoint and 30-FPS screen.
