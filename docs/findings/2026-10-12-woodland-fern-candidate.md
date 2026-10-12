# Woodland fern clump candidate (2026-10-12)

Added `Scripts/AssetPipeline/assets/woodland_fern.py`, a no-credit,
procedurally authored fern for woodland-edge and shaded-ground variety. The
candidate has six uneven arching fronds, individually modelled leaflets, a
small grounded crown, and no collision so it cannot obstruct character paths.
It uses two parameter-only green materials; it does not depend on generated
textures, ComfyUI, or the shared asset-pipeline helper changes.

The Blender 5.2.2 export/re-import preview completed with `PROBLEMS []`, one
`UVMap`, two material slots, exact `ZMIN 0.0`, dimensions 110 × 108 × 60 cm,
and 5,912 triangles. The FBX tangent warning from the first, spindly draft was
removed by triangulating the finished meshes; the current build log contains
no exporter tangent warning. The [verified preview](../../Saved/CaptiveSky/ComfyBlender/WoodlandFernClump/Preview.png)
shows a stylized, open fan silhouette that should add a distinct leaf shape
among low vegetation. It is a mesh/material candidate, not finished natural
art.

No Unreal asset was imported and no PCG graph or Island placement was changed.
Wind response, Unreal material appearance, LODs, instanced draw cost, and
composition among existing foliage are all unverified. Next gate: import this
single FBX into a temporary generated-content folder when the editor is stable,
review it at ground and normal play distances, then test a tiny HISM/PCG patch
in an isolated preview before considering Island integration.
