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
  disabled and a zero model-request limit. The test checks the local Content
  file: if the `.uasset` is installed, the detailed mesh must actually load and
  be visible at the intended scale, have no collision, and hide the primitive
  shell; if it is absent, the primitive fallback must stay visible. The
  strengthened test passed with the local asset installed
  ([log](../../Saved/Logs/Codex_TidepoolCrabAssetLoaded_20261013.log)).

A bounded 60.2-second real-RHI Game session then loaded the saved Island at
12:00 with an isolated data root, Python and agent thinking disabled, and a
zero-request cap. It exited normally with zero model requests. Three spectator
frames are saved under
[`Codex_TidepoolCrabRuntime_20261013`](../../Playtests/Codex_TidepoolCrabRuntime_20261013/Screenshots/003_Tideglass.png);
the last frame shows the real runtime water and active meadow/woodland fauna.
That wide establishing view does **not** make the small crab individually
legible, so it validates loading and overall composition, not the crab's final
in-world art read. The log also reports many installed rhododendron material
instances missing the `InstancedStaticMeshes` usage flag and falling back to
the default material; that is a separate foliage-material issue, not changed
here.

The local round-trip preview is
`Saved/CaptiveSky/ComfyBlender/TidepoolCrab/Preview.png`. It is a source-art
review, not a close real-RHI in-world capture. The asset remains static and has
not been reviewed at crab-level camera distance or for motion readability; do
not treat the silhouette, scale, or color as final art approval yet. Content
and FBX outputs are gitignored, so other clones retain the primitive fallback
until the candidate is imported there too.

## Next step

Add a dedicated, close real-RHI crab view that keeps the subject on dry shore;
review its material and scale against the wet sand/rocks, then decide whether
its idle/scurry movement reads naturally. If not, revise the Blender source and
import to a fresh candidate destination. Keep map placement separate so this
art comparison remains reversible.
