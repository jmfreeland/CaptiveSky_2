# Common Frog visual candidate

## Decision

Create a small, collisionless visual candidate for possible use at the damp
Tideglass margins. This is an offline asset-pipeline iteration only: it is not
imported into Unreal, placed in the Island, registered as a resident, or given
behavior. The user said the Unreal editor is currently open and responsive; no
editor interaction was needed or attempted for this mesh review.

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
- Status: offline candidate verified. Unreal import, visual scale in the Island,
  habitat placement, species behavior, and runtime cost remain unverified.

## Safety and next gate

Keep the candidate out of the map until its exported bounds, UVs, material slots,
and triangulation pass offline verification. If approved after editor review,
integrate later as a small ambient resident with an explicit damp-margin habitat
and bounded non-LLM behavior; do not infer that this standalone mesh completes
that integration.
