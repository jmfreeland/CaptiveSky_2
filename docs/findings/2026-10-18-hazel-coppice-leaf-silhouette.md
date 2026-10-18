# Woodland hazel coppice leaf-silhouette iteration (2026-10-18)

The existing `WoodlandHazelCoppice` export read as oversized, angular leaf
plates in its Blender preview. The source now uses a seven-point broad-ovate
leaf width profile rather than a five-point diamond-like profile, with a
shallow central ridge and small deterministic edge variation. It remains a
stylized, collisionless woodland-edge clump; this iteration does not add it to
the Island.

## Validation

- Blender 5.2.2 rebuilt the source into the isolated local folder
  `Saved/CaptiveSky/ComfyBlender/Codex_WoodlandHazelCoppiceLeaf_20261018/`.
- The exported-FBX verifier re-imported the mesh and rendered
  `WoodlandHazelCoppice/Preview.png`: 17,910 triangles, bounds 169 × 192 × 258
  cm, no collision shapes, one `UVMap`, four material slots, and `PROBLEMS []`.
- The previous local export had 12,310 triangles and the same rounded bounds;
  this silhouette refinement increases geometry by about 45%.
- The focused source contract passed (2 tests) using Blender's bundled Python.
  A full asset-pipeline test discovery was also attempted, but that sandboxed
  run is not a valid suite result: it cannot write to its system temporary
  directory, and the Blender Python environment lacks Pillow. The repository's
  documented system `python`/`py` launchers are unavailable here.

## Review gate

This is a Blender preview and FBX round-trip check only. No Unreal import,
Island placement, real-RHI lighting review, or gameplay-scale comparison was
performed because the live editor could not be targeted. Keep the candidate
isolated until the editor is available, then compare the previous and revised
mesh at the same woodland-edge site and lighting. Approve it for placement only
if the smaller leaves retain enough mid-distance canopy mass and the clump
reads distinctly from the existing hazel-thicket candidate.
