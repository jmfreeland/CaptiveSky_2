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
- Unreal's initial isolated import exposed smoothing-group and zero-tangent
  warnings. The FBX exporter now writes smoothing groups and omits its
  unreliable tangent-space payload; the leaf tooth variation also scales with
  the local leaf profile so tip triangles do not collapse to near-zero area.
- Blender's FBX round-trip tangent check now reports zero near-zero tangents.
  The focused source contract passes (3 tests), including the export settings
  and silhouette-detail scaling checks.
- The corrected FBX imported into a fresh isolated Unreal folder,
  `/Game/Generated/ComfyBlender/Codex_WoodlandHazelCoppiceTangents_20261018/`.
  UE 5.8 built a 17,910-triangle mesh (169 × 192 × 258 cm), with four material
  slots and no collision shapes. The import log contains no smoothing,
  tangent, or bitangent warnings for this candidate; the import report has
  `validation_errors: []`.
- The Static Mesh Editor preview was visually checked in the live editor. The
  clump reads as a compact stylized hazel shrub. The Island map was not edited
  or saved.
- The previous local export had 12,310 triangles and the same rounded bounds;
  this silhouette refinement increases geometry by about 45%.
- A follow-up mid-distance budget pass keeps six longitudinal samples per
  leaf (instead of seven) with a broad, rounded profile. The isolated Blender
  5.2.2 export at
  `Saved/CaptiveSky/ComfyBlender/Codex_HazelLowPoly6_20261018/` verifies at
  15,110 triangles, 169 × 192 × 258 cm, four material slots, one `UVMap`, no
  colliders, and `PROBLEMS []`. Compared with the 17,910-triangle silhouette
  refinement, this reduces geometry by 15.6%; the CPU re-import preview retains
  the same compact, open-centred shrub read. The source contract passes (3
  tests). This is still only a local candidate: it has not been imported into
  Unreal or placed on the Island.
- A full asset-pipeline test discovery was also attempted, but that sandboxed
  run is not a valid suite result: it cannot write to its system temporary
  directory, and the Blender Python environment lacks Pillow. The repository's
  documented system `python`/`py` launchers are unavailable here.

## Current review gate

The isolated source/export candidate passes the Unreal import and visual-preview
checks. It remains unplaced: no Island-site comparison, real-RHI lighting
review, gameplay-scale comparison, or ecology review has been performed. Keep
it isolated until those checks can be made against the existing hazel-thicket
candidate at a representative woodland-edge site.
