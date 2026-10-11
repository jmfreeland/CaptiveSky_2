# Rat Hollow prop review

## Current asset

- Unreal asset: `/Game/Generated/ComfyBlender/RatHollow/SM_RatHollow_01` (StaticMesh).
- Source and import report: `Saved/CaptiveSky/ComfyBlender/RatHollow/`; `unreal_import.json` reports no validation errors.
- Unreal registry: 1,602 vertices, 1,836 triangles, four material slots (wood, dark hollow, straw, moss), four simple box collision shapes, two UV channels, Nanite disabled; approximate bounds 256 × 136 × 134 cm.
- Read-only Static Mesh Editor inspection shows a weathered fallen log and a clearly distinct circular hollow entrance; the source asset also includes moss patches and a small straw nest bed. The default side thumbnail hides the entrance; orient it toward the Rat's approach/habitat when placing it.
- Live read-only bounds plus `Scripts/Create-RatPlaceholder.py`: `/Game/Characters/Creatures/Rat/SK_Rat_01` measures about 22 × 112 × 38 cm in local X/Y/Z and is scaled to 0.28 in the Rat Blueprint. The mesh's long Y axis is rotated to the hollow's X-aligned tunnel; its scaled 6 × 31 × 11 cm bounds and the Blueprint's 16 × 24 cm capsule fit the hollow's nominal 78 cm entrance and 80 × 84 cm collision corridor. This is a geometric fit, not a traversal result. The skeletal mesh has no Physics Asset, which does not prevent capsule-based Character movement but would matter for ragdoll or mesh-driven physics.

### Entrance-facing source preview (2026-10-11)

The asset verifier accepts an optional `ASSET_PREVIEW_DIRECTION` vector from mesh centre toward the camera; its default composition is unchanged. For `RatHollow_TornEntranceV2`, the entrance is on local `-X`, so `-1,0,0` produces the saved [`Preview-Entrance.png`](../../Saved/CaptiveSky/ComfyBlender/RatHollow_TornEntranceV2/Preview-Entrance.png). The frontal render makes the irregular bark rim and small straw nest bed legible. The depth reads very dark in this simple Eevee preview; check the hollow under actual Island lighting before deciding whether it needs an interior bounce/light treatment. The companion `Preview-Side.png` confirms the weathered trunk profile but is not an in-world lighting or placement review. No source geometry, Unreal asset, or Island state changed for these preview renders.

### Warm entrance readability pass (2026-10-11)

The source generator now uses its dark-earth material instead of near-black
charcoal for the cavity backing, and enlarges the straw bed modestly so the
nest is more legible from the entrance. This leaves the trunk dimensions,
entrance geometry, and all five collision boxes unchanged. The source-level
RatHollow suite passes 5/5, including capsule clearance and the new material /
bedding contract. The source was exported and re-imported in isolated review
storage at
`Saved/CaptiveSky/ComfyBlender/Codex_RatHollowCurrentReview_20261011/`:
262 × 140 × 133 cm, 1,994 triangles, five box colliders, UVMap, four material
slots, and `PROBLEMS []`. The new [entrance preview](../../Saved/CaptiveSky/ComfyBlender/Codex_RatHollowCurrentReview_20261011/RatHollow_TornEntranceV2/Preview.png)
shows a warm brown recess and a more visible nest bed, rather than the prior
black disc. This is still a simple Blender review render; Unreal lighting,
material import, Rat traversal, and placement remain unverified. The existing
Unreal trial asset and Island map were not modified.

### Ground-level collision revision (2026-10-11)

The source-level `RatHollow` generator previously put a full-width box across the tunnel floor, creating the raised sill described above. `Scripts/AssetPipeline/assets/rathollow.py` now uses two narrow ground runners at the sides instead, preserving a 48 cm central gap while the side and roof boxes still describe the shell. `Scripts/AssetPipeline/tests/test_rathollow.py` passes a source-level swept-AABB check for the Rat's nominal 16 × 24 cm capsule from the entrance into the tunnel (2 tests passed).

The corrected source was exported and re-imported by Blender's asset verifier in a separate disposable folder, `Saved/CaptiveSky/ComfyBlender/RatHollow_ClearanceTrial_20261011_01/`. The verifier found unchanged 256 × 136 × 134 cm bounds, the expected four material slots and UV layer, five UCX boxes, and `PROBLEMS []`. The export/round-trip checks asset structure, not Unreal CharacterMovement, NavMesh, or the visual fit in the Island.

The original `/Game/Generated/ComfyBlender/RatHollow/SM_RatHollow_01` remains unchanged. The corrected generator has since been imported as a separate local Unreal trial asset; the Rat has not traversed it in UE, so runtime walkability remains unverified. The Unreal Editor is currently closed, and no placement was made; do not update `places.json` from these asset checks alone.

### Unreal trial import and traversal status (2026-10-11)

The corrected pack from `Saved/CaptiveSky/ComfyBlender/RatHollow_ClearanceTrial_20261011_01/` is imported under `/Game/Generated/ComfyBlender/RatHollow_ClearanceTrial/`, separate from the original. [`unreal_import.json`](../../Saved/CaptiveSky/ComfyBlender/RatHollow_ClearanceTrial_20261011_01/RatHollow/unreal_import.json) reports bounds of 256.4 × 135.7 × 134.1 cm, all four material slots assigned, five box-collision elements, and no validation errors. UE's asset checks validated the mesh, four materials, and imported textures. This confirms content structure only; no live material/lighting inspection or Rat traversal has occurred, and no Island map change was made.

The first UE commandlet attempt failed while creating a shader transfer file in the default `C:/Users/freel/UnrealShaderWorkingDir`. A retry with `-shaderworkingdir` under `Saved/CompileScratch/` but without `-NullRHI` began compiling SM6 engine materials and was stopped after its log remained unchanged for over a minute at about 4.6 GiB. A `-NullRHI` attempt with a relative Python path exited normally but resolved the script relative to the engine directory and did not import anything. The successful invocation used `-NullRHI`, the project-local shader path, and an absolute Python script path; see [`asset-import-pipeline.md`](../asset-import-pipeline.md#comfyblender-asset-import). Do not overlap Unreal commandlets. Next, open the trial asset in the live editor, inspect its entrance/collision and appearance, then run the bounded site audit and physically test the Rat before any placement or memory update.

## Relationship to Fenrus

`Agents/Agent_Rat_01/identity.md` says the Rat's first home is a hidden fallen-wood hollow. `Agents/Agent_Rat_01/places.json` is still empty, so no in-world coordinates are claimed yet. The asset is imported and visually readable, but has not been placed or traversability-tested with a Rat body.

The user has confirmed the home concept: a hidden fallen-wood hollow. This confirms the habitat/design, not a particular Island site; keep `places.json` empty until a sheltered, dry, accessible location is verified in-world. The current editor viewport is a broad, exposed patch of bare ground, so it is not a home candidate.

A read-only live viewport survey at `(-100500, 102000)` showed open grass beside the inn with little nearby cover; a higher survey near `(-100000, 101000)` showed only a sparse tree group. Reject this near-inn patch as a hidden home candidate. The island needs more understory/vegetation before a convincing sheltered rat home can be selected; no RatHollow placement was made.

A newer read-only capture of the unsaved live map found `Roost_East_FallenWood` at approximately `(-99850, 101980, 2767)`, but its component uses `/Engine/BasicShapes/Cylinder` at scale `(0.55, 0.55, 5)`, with bounds about 299 × 463 × 98 cm. In the close view it reads as a bright, smooth roost prop over fairly sparse grass, not as a natural hollow or convincing hidden home. `Roost_East_MainBranch` is also a stretched BasicShapes cylinder. `Prop_InnFirewood` is a separate Tripo firewood stack at the inn, not a secluded habitat. Keep all three as visual leads only; no transform or level data was changed, and this live unsaved-map survey is not evidence about the saved map. The read-only candidate-classifier regressions pass 4/4, but they test hint classification only—not map discovery, habitat quality, or navigation.

## Placement safety

The user confirmed that authoring changes were saved. The Unreal Editor is now closed, and no placement was made. Do not run `Scripts/Place-IslandProps.py` until its replacement behavior is narrowed or a specific safe workflow is established: a normal run replaces every actor tagged `GeneratedProp`/`TripoProp` and saves the level. First regenerate/reimport the corrected hollow collision, then choose a hidden, dry, Rat-accessible ground point, validate the approach/orientation and collision, and only then create the home placement and update Rat place-memory.

The bounded read-only `Island.RatHollowSiteAudit` command is implemented in `Source/CaptiveSky_2/Agent/IslandNavProbe.cpp` (commit `c696305`) and only runs in a Game world. The editor is currently closed; its module version, site shortlist, visual habitat quality, navigation, and physical traversal remain unverified. Once reopened, run the site audit in one bounded diagnostic session, visually inspect its suggestions, and physically test the Rat's approach before recording coordinates. The scan itself does not save or modify the world and its output is only a site suggestion, not placement approval.
