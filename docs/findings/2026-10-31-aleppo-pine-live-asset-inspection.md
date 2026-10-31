# Aleppo pine live asset inspection — 2026-10-31

## Observation

After the user-approved editor restart, the `CaptiveSky_2` editor is open on
the Island and reports **All Saved**. In the live Content Drawer, the installed
package `/Game/Megaplant_Library/Tree_Aleppo_Pine/Tree_Aleppo_Pine_01` contains
four visible skeletal-mesh variants (A–D). Variant A's Unreal tooltip reports:

- skeletal mesh, 6.869 MiB on disk;
- 2,688 bones, one LOD, 4,128 base triangles / 12,384 vertices;
- Nanite Enabled, with 355,485,125 Nanite triangles and 352,010,582 Nanite
  vertices; estimated Nanite compressed size is 0 B.

The thumbnail silhouette appears distinctly narrower/coniferous than the
existing broadleaf Megaplants, so it could improve the Island's tree silhouette
variety. This is editor-visible metadata, not proof that it loads correctly in
the runtime foliage provider, has usable Dynamic Wind data, renders well at
Island scale, or is affordable in memory. The zero-byte Nanite compressed-size
estimate alongside the reported hundreds of millions of Nanite triangles is
especially a reason to inspect the asset and its render path before placement.

## Decision and next gate

Treat Aleppo Pine as a promising but unqualified species. Do not increase the
3,600-tree broadleaf budget or place a full population from this mesh yet. The
existing woodland code expects foliage-compatible assets through
`UInstancedSkinnedMeshComponent`, and earlier Megaplant probes found that
imported meshes could have sparse/dark canopies and undesirable Nanite
appearance. The project also has prior video-memory exhaustion warnings.

Next, use a disposable preview in the editor to verify the Aleppo materials,
wind data, foliage silhouette, and Nanite/non-Nanite appearance; then measure a
small instanced batch against the same existing Island camera view. If that
passes, add a small capped share under the existing total-tree budget, preserve
the current fallback when the asset is unavailable, and validate mixed-species
counts, FPS, and peak VRAM before calling it a production tree.

This inspection made no map, package, or source changes in the live project.
