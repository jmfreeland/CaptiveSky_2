# Common Frog visual candidate

## Decision

Create a small, collisionless visual candidate for use by the existing
transient Common Frog pair at the damp Tideglass margins. The candidate has now
been imported into a new local Unreal Content folder, but has not been placed
in the saved Island map or registered as a persistent resident. Existing
source behavior remains bounded and non-LLM.

## Species grounding

The candidate is a Common Frog (*Rana temporaria*). ARC notes that the species
is widespread in Britain and Ireland, occurs in many habitats near breeding
ponds, and may reach about 9 cm; olive/brown coloration, a dark eye mask, and
variable dark markings are reflected as restrained visual cues rather than a
fixed exact pattern. See [ARC: Common Frog](https://www.arc-trust.org/common-frog).

## Validation gate

- Model source: `Scripts/AssetPipeline/assets/common_frog.py`.
- Focused source contract: `Scripts/AssetPipeline/tests/test_common_frog.py`.
- Intended scale: roughly 9 cm snout-to-rear-foot, authored in metres.
- Geometry: crouched body, broad head, eye masks and ridges, dorsal mottles,
  folded forelegs, bent hind legs, and short toes; no collision geometry.
- Blender 5.2.2 FBX export and re-import verifier: passed; 1,464 source
  triangles, 1,324 imported polygons, one `UVMap`, eight material slots, no
  collision, 9.2 × 7.9 × 4.1 cm bounds, Z minimum 0.0022 m, and `PROBLEMS []`.
- Focused Python source contracts: 3/3 passed. Full AssetPipeline suite: 92/92
  passed (Python 3.13 bundled runtime; workspace-local temp directory).
- Preview: `Saved/CaptiveSky/ComfyBlender/Codex_CommonFrog_20261018/`
  `Tideglass_CommonFrog_Candidate/Preview.png`. Surface-following dorsal marks
  were adjusted after visual inspection of the first render.
- Status before Unreal import: offline candidate verified; Island appearance,
  habitat fit, and runtime cost remained unverified.

## Safety and next gate

The existing Tideglass subsystem spawns up to two transient frogs from the
pool footprint when a fresh world begins; these actors do not alter the saved
map. The new mesh is intended for that existing behavior, not for a new placed
actor. In-world material/lighting readability and the pair's gameplay-distance
appearance still need a visual review before treating the candidate as art-
approved.

## Unreal import and focused behavior validation (2026-10-19)

The documented `Scripts/Import-ComfyBlenderAsset.py` pipeline imported the FBX
to `/Game/Generated/ComfyBlender/Tideglass_CommonFrog_Candidate/` without
opening or saving the Island map. The local import report records a 9.2 × 7.9 ×
4.1 cm static mesh, all eight manifest material slots assigned, zero collision
shapes as intended, and `validation_errors: []`. Unreal warned that the FBX
contains no smoothing-group metadata; the importer still succeeded. The
imported `.uasset` lives under gitignored `Content/`, so this is a local project
asset and does not travel with source commits.

The first scratch automation pass loaded the imported package without the old
`IslandCommonFrog` missing-mesh constructor error, then found that
`HopArcHeight(1.f, 13.f)` returned a tiny nonzero `sin(PI)` residue. The
function now returns exact ground height at both clamped arc endpoints. An
incremental UE 5.8.3 scratch editor-target build succeeded (four actions), and
the focused `CaptiveSky2.Agent.CommonFrogEcology` test passed afterward.
The grouped `Frog` automation run also passed all five registered tests:
`CommonFrogEcology`, `CommonFrogVisitorInteraction`,
`CommonFrogDragonflyAttention`, `DragonflyFrogResponse`, and
`DragonflyFrogResponseWorld`. These NullRHI checks used a scratch DLL, disabled
resident thinking, and allowed zero model requests; they validate code and
package loading, not rendered appearance. The live editor stayed open and its
loaded class defaults were not reloaded, so visual use by the current session
remains unverified until a normal editor/game reload.
