# Tideglass soft-rush candidate — 2026-10-17

Added a deterministic, collisionless soft-rush-inspired wet-edge clump to the
local Blender asset pipeline. Its upright culms and small side seed-panicles
provide a taller, narrower silhouette than the sea-thrift and wind-combed
marram candidates. Geometry uses two flat material slots and no generated
textures; it adds no collision, navigation changes, runtime actors, model
requests, or Tripo credits.

The source contract passed all three focused tests. Blender 5.2.2 exported
`Saved/CaptiveSky/ComfyBlender/TideglassSoftRush/SM_TideglassSoftRush_01.fbx`
with 59 x 61 x 91 cm bounds, 4,928 triangles, two material slots, and a 0.38
cm ground-contact minimum. The first version was 11,200 triangles; reducing
the seed-panicle mesh brought it below a 5,000-triangle per-clump target. The
pipeline verifier re-imported the FBX, retained `UVMap`, found no collision,
and reported `PROBLEMS []`. Its preview is
[`Preview.png`](../../Saved/CaptiveSky/ComfyBlender/TideglassSoftRush/Preview.png).

This remains a local candidate outside `Content/` and is not placed on the
Island. The Blender preview suggests the intended upright silhouette, but does
not establish that the pale stems and seed heads fit the actual Tideglass
lighting, shoreline composition, or player view. When the editor is safe to
use, import into a fresh generated-asset destination and compare a sparse
five-to-ten-clump strip from the fixed Tideglass Game viewpoint. Check that it
reads as wetland vegetation rather than a bamboo-like thicket, preserves the
shoreline sightline, and stays within the project's frame-rate floor before
considering any saved-map placement.
