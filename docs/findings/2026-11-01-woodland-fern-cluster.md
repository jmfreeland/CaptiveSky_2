# Woodland lady-fern understory candidate — 2026-11-01

The woodland understory now has a distinct fern-form candidate to compare with
the existing low groundcovers and fungi. `Scripts/AssetPipeline/assets/woodland_fern_cluster.py`
builds a deterministic rosette of seven curved fronds with paired, softly
folded pinnae and four restrained green material slots. The model is
collisionless and adds no Island instances until it is imported and deliberately
placed.

Blender 5.2.2 exported and re-imported `SM_WoodlandFernCluster_01.fbx` with
`PROBLEMS []`: 85 × 87 × 40 cm, 1,974 triangles, four material slots, one
`UVMap`, and no collision. The exported preview reads clearly as a fern, though
its clean, faceted leaflets remain stylized. This geometry-only preview does not
establish how it will read under Island lighting, among the existing grass, or
at visitor distance.

The FBX and `Preview.png` are local ignored outputs at
`Saved/CaptiveSky/ComfyBlender/WoodlandFernCluster/`. The asset is **not imported
into Unreal or placed on the Island**. The staged workflow in
`Scripts/Preview-WoodlandFlyAgaricReview.py` now imports both this fern and the
red-cap candidate, then places them on opposite sides of the existing ochre
cluster in an isolated Island copy at
`/Game/Generated/ComfyBlender/WoodlandUnderstoryReview_20261101/Maps/Island_WoodlandUnderstory_20261101`.
Review matching close and visitor-scale views there; keep the fern unplaced
unless its silhouette, ground contact, material response, and grass occlusion
all hold up under Island lighting.

Source-level geometry/material tests are in
`Scripts/AssetPipeline/tests/test_woodland_fern_cluster.py`. Four focused
source-contract tests covering this fern and the neighboring fly-agaric
candidate pass inside Blender 5.2.2's bundled Python. The six tests in
`Scripts/tests/test_woodland_fly_agaric_review.py` also pass there, covering
the two-asset preflight, candidate offsets, and copy-only map save. Exported-FBX
verification also passed; no Unreal import, map, runtime, or performance check
is claimed.
