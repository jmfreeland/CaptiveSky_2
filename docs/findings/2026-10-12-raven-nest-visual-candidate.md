# Raven woven-nest visual candidate — 2026-10-12

## Candidate

Added `Scripts/AssetPipeline/assets/raven_nest.py`, a deterministic Blender
builder for a small, shallow woven nest. It keeps the occupant space open,
uses a matte two-color bark/grass palette, has no collision, and is intended as
a reversible visual candidate—not a new nest mechanic or a map placement.

The FBX was built and re-imported by Blender 5.2.2's pipeline verifier. Current
measurements are 45 × 44 × 17 cm, 3,708 triangles at export (2,124 polygons
after verification import), one `UVMap`, two material slots, zero collision
shapes, and `ZMIN 0.0`. The verifier reported `PROBLEMS []`. The source script
also triangulates n-gons before export to avoid the tangent warning seen in its
first build.

## Review limits and next step

The render is a pipeline preview, not Unreal/gameplay evidence. The Unreal
editor and MCP bridge were unavailable during this pass, so the mesh was not
imported into `/Game`, attached to the persistent nest materializer, or placed
in the Island. Do not replace the current staged, procedurally woven nest until
the imported candidate has been compared at Raven scale and at normal visitor
distance; a static replacement must also preserve visible weave-stage and
storm-debris progression.

Preview: `Saved/CaptiveSky/ComfyBlender/RavenNest_WovenBowl/Preview.png`.
Export: `Saved/CaptiveSky/ComfyBlender/RavenNest_WovenBowl/SM_RavenNest_WovenBowl_01.fbx`.

## Moss-lined variation — 2026-10-18

Added a separate `raven_nest_moss_lined.py` variation rather than altering the
original candidate. It reuses the same open woven bowl and adds 18 short,
curved muted-green strands beneath the dry-grass lining. This keeps the
occupant space open and preserves the original weave geometry; it does not
change the live nest actor, construction stages, storm debris, persistence, or
the Island map.

Blender 5.2.2 exported and re-imported the variation successfully: 45 × 44 ×
17 cm, 4,500 triangles before verification import, one `UVMap`, three material
slots, zero collision shapes, `ZMIN 0.0`, and `PROBLEMS []`. The regenerated
[pipeline preview](../../Saved/CaptiveSky/ComfyBlender/Codex_RavenNestMossLined_20261018/RavenNest_WovenBowl_MossLined/Preview.png)
shows a subtle green fiber layer at close range, but this remains a Blender
preview—not Unreal lighting, imported material, or in-world evidence. Keep the
current procedural nest presentation until this variation is imported to a
disposable Unreal project and compared at Raven scale; the staged weave and
storm-debris behavior must remain intact if it is ever used as a base mesh.

The focused source-contract checks pass 3/3. The full AssetPipeline suite
passes 89/89 when `TEMP` and `TMP` point into the project's ignored
`Saved/CompileScratch/` area; the default Windows app-sandbox temporary folder
denied writes during the first attempt. Neither check proves the variation is
attractive in the Island or ready to replace the live materializer.
