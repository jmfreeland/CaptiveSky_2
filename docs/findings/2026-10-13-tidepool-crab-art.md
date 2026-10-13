# Tideglass tidepool crab art candidate

## Change

Replaced the Tidepool Crab's purely primitive appearance with an optional,
locally authored static-mesh candidate. The source is
`Scripts/AssetPipeline/assets/tidepool_crab.py`; it uses plain material colors
and no generated textures or paid service. The low-poly russet crab has a domed
carapace, segmented walking legs, paired claws, and eye stalks. The exported
mesh is about 80 × 85 × 37 cm with 1,940 triangles and six material slots.

The FBX is imported to the isolated path
`/Game/Generated/ComfyBlender/IslandTidepoolCrab/SM_TidepoolCrab_01`. The
import report records no texture imports, no collision shapes, and an empty
`validation_errors` list. It is not placed by a map edit. The actor loads it
when present and hides its old primitive parts; a clean checkout or a machine
without the local, gitignored Content asset keeps the portable primitive
fallback. The candidate component has collision and navigation influence
disabled.

## Validation

- Blender's FBX re-import verifier completed and refreshed the local preview.
- Unreal's import report measured the mesh at 79.9 × 84.8 × 36.8 cm and found
  no validation errors or collision geometry.
- `CaptiveSky_2Editor Win64 Development` built successfully with UE 5.8.3.
- `CaptiveSky2.Agent.NightEcology` passed under NullRHI with agent thinking
  disabled and a zero model-request limit. The test confirms the detailed mesh
  is loaded and visible at the intended scale when present, has no collision,
  and leaves the primitive fallback visible when the mesh is absent. See the
  [automation log](../../Saved/Logs/Codex_TidepoolCrabNightEcology_20261013.log).

The local round-trip preview is
`Saved/CaptiveSky/ComfyBlender/TidepoolCrab/Preview.png`. It is a source-art
review, not a real-RHI in-world capture. The asset remains static and has not
been reviewed in the Tideglass shoreline composition or under gameplay camera
distance/lighting; do not treat the silhouette, scale, or color as final art
approval yet. Content and FBX outputs are gitignored, so other clones retain
the primitive fallback until the candidate is imported there too.

## Next step

Review a close real-RHI Tideglass capture with the mesh enabled, then a normal
gameplay-distance view. If its scale and russet palette read naturally against
the wet sand and rocks, keep it; otherwise revise the Blender source and import
to a fresh candidate destination. Keep movement/animation and placement as
separate follow-ups so this art comparison remains reversible.
