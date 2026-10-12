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

**Unreal import validation (2026-10-12):** the UE 5.8.3 commandlet importer
placed the FBX and its two parameter-only materials in the new local folder
`/Game/Generated/ComfyBlender/WoodlandFern_Review_20261012`. The measured bounds
were 110.1 × 108.2 × 59.7 cm, both material slots resolved, no collision was
created, and `validation_errors` is empty. The unique destination avoided any
reimport or replacement; no map was loaded or saved. Evidence:
[`unreal_import.json`](../../Saved/CaptiveSky/ComfyBlender/WoodlandFernClump/unreal_import.json)
and [`import log`](../../Saved/Logs/Codex_WoodlandFernImport_20261012.log).

This was a `NullRHI` import check, not a visual Unreal review. Wind response,
real-RHI material appearance, LODs, instanced draw cost, and composition among
existing foliage remain unverified. Next gate: render the single imported mesh
in an isolated real-RHI preview at ground and normal play distances, then test a
tiny HISM/PCG patch before considering Island integration.
