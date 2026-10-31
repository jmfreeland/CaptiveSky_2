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

## Six-species metadata preflight (2026-10-31 22:34 UTC)

Added `Scripts/Inspect-MegaplantAssetRegistry.py`, a fast, read-only
Asset-Registry inventory that enumerates package paths and classes without
loading mesh/PVE objects or building Nanite data. Against the isolated UE 5.8.3
scratch project, whose `Content` is a read-only junction to the main project,
it reported 371 registry entries: 24 A–D skeletal tree variants and 18 PVE
assets across Aleppo Pine, Black Alder, Common Hazel, European Aspen, European
Beech, and Norway Maple (four mesh variants and three PVE-related assets per
species). The run used NullRHI and did not load the Island. Log:
`Saved/CS58/Saved/Logs/Codex_MegaplantRegistryAudit_20261031.log`.

The existing detailed inventory was also tried against this read-only Content
view, but loading Aleppo variants began on-demand skeletal/Nanite work and
remained at `Waiting for skinned assets to be ready 0/1` for over two minutes.
That isolated commandlet was stopped before its report completed. The script
made no explicit save, and the live project/map remained untouched. This is a
concrete reason to use registry-only preflight under memory pressure; it is
availability evidence only, not a successful mesh-load or render check. The
incomplete detailed run is preserved at
`Saved/CS58/Saved/Logs/Codex_MegaplantFullInventory_20261031.log`.
