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
