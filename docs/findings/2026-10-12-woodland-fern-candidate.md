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
existing foliage remained unverified at that point.

**Isolated Island-copy Game review (2026-10-12):** a two-stage editor script
duplicated `/Game/Maps/Island` to a fresh local review map and placed one
no-collision fern actor. The original Island was never saved. The first capped
Game run (90.2 seconds; zero model requests) showed the plant lost inside an
existing dense HISM patch, so the review actor was moved within the disposable
copy to an adjacent clear patch. A second Game run (60.1 seconds; zero model
requests) produced close and woodland-edge views. The fern's two green material
slots render correctly, and the open fan silhouette is recognizable, but the
asset is distinctly low-poly and casts an unusually sharp, opaque star-shaped
shadow. It reads as a solitary specimen in a bare opening, not a proven natural
woodland group. No frame-time profile or HISM/PCG comparison was run.

The review copy's StaticMeshComponent had `cast_shadow=true`; a commandlet then
saved `cast_shadow=false` for that actor only. The follow-up Game process failed
to reach world-ready within the separate 300-second startup watchdog and was
terminated; it produced no shadow-off image, so the visual effect of that
change is unverified. The copy also logged a missing `/Script/WaterEditor`
import warning, though the fern and surrounding ground cover rendered in the
previous Game runs.

Evidence: [clear-patch close view](../../Saved/Playtests/Codex_WoodlandFernGameClearPatch_20261012/Screenshots/001_Clear_Patch__Close.png),
[woodland-edge view](../../Saved/Playtests/Codex_WoodlandFernGameClearPatch_20261012/Screenshots/002_Clear_Patch__Woodland_Edge.png),
[zero-request Game log](../../Saved/Logs/Codex_WoodlandFernGameClearPatch_20261012.log),
[shadow-off startup log](../../Saved/Logs/Codex_WoodlandFernShadowOff_20261012.log).

Keep the fern as an isolated visual candidate; do not add it to the Island yet.
Next gate: verify the shadow-off appearance in Game, then evaluate a small,
sparse HISM patch with the tested material and spacing. Compare close and
mid-distance readability and measure the patch before considering any Island
integration. Preserve the 30-FPS floor and do not compensate for bare
midground by globally increasing foliage counts.
