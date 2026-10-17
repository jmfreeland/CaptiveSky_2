# Tideglass sea-thrift candidate — 2026-10-17

Added `Scripts/AssetPipeline/assets/tideglass_sea_thrift.py` as a local,
deterministic shoreline-groundcover candidate. The tuft has six slender flower
scapes, small five-petal pink florets, and four narrow basal leaves per scape.
Its plain materials and collision-free geometry suit repeated static-mesh
instances beside the salt-sprayed Tideglass pool; it adds no model call,
runtime actor, saved-world state, or player collision.

The focused source contract passed all three tests. Blender 5.2.2 built the FBX
at `Saved/CaptiveSky/ComfyBlender/TideglassSeaThrift/SM_TideglassSeaThrift_01.fbx`:
83 × 70 × 43 cm bounds, 3,846 triangles, four material slots, and Z-min 0.57 cm.
The pipeline verifier re-imported that FBX and reported `UVMap`, no collision,
and `PROBLEMS []`. A CPU-only, 8-sample reimport render is at
[`Preview.png`](../../Saved/CaptiveSky/ComfyBlender/TideglassSeaThrift/Preview.png).
The rendered candidate reads as a low, dispersed tuft with clustered pink
flower heads; it has not been seen under Unreal lighting or beside actual
Tideglass rocks and vegetation.

The candidate is not imported into `Content/` or placed in `/Game/Maps/Island`.
Keep it local until the editor can be inspected again, then import to a fresh
generated-asset destination, check scale/material readability in Unreal, and
only then consider a small, non-blocking shoreline patch. Its current preview
uses a simple test ground and is not evidence of an Island composition.

## Rosette refinement — 2026-10-18

The first preview showed the basal leaves as dark, flat, crossing blades. The
source now uses a lighter coastal green, shorter 13–18 cm leaves, and a modest
raised arch through each leaf surface. This keeps the small pink flower heads
as the focal point while letting the basal tuft read separately from a flat
starburst.

The focused source contract passes 4/4 tests. Blender 5.2.2 built and
re-import-verified a separate review export at
`Saved/Playtests/Codex_SeaThriftRosette_20261018/ComfyBlender/TideglassSeaThrift/`:
66 × 56 × 43 cm, 3,846 triangles, four material slots, `UVMap`, no collision,
and `PROBLEMS []`. The CPU preview is
[`Preview.png`](../../Saved/Playtests/Codex_SeaThriftRosette_20261018/ComfyBlender/TideglassSeaThrift/Preview.png).
The existing `Saved/CaptiveSky/ComfyBlender/TideglassSeaThrift` export was not
overwritten. This is still a Blender-only candidate; Unreal material/lighting,
shoreline fit, gameplay-distance readability, shadow cost, and the 30-FPS p95
gate remain unverified. Keep it out of `Content/` and the Island map pending
that review.
