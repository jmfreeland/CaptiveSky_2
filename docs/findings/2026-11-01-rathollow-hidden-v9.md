# RatHollow Hidden V9 candidate

V9 is a reversible source-and-preview iteration on the hidden fallen-wood home.
It is not imported into Unreal or placed in the Island. V7 remains the structural
baseline; see the [V7 review](2026-10-31-rathollow-hidden-v7.md) and
[V8 review](2026-11-01-rathollow-hidden-v8.md).

## Changes and export

The scripted Blender source,
`Scripts/AssetPipeline/assets/rathollow_hidden_v9.py`, keeps V7's five box colliders,
four material slots, and named entrance/nest anchors. It removes the three exposed
entrance-tip groups, replacing them with a low canopy and short asymmetrical root
shoulders that stay behind the front plane; low side roots remain tucked within the
trunk's silhouette. The anchor locations are unchanged: `entrance` at `[-132, 0, 66]`
cm and `nest_bed` at `[-32, 0, 32]` cm.

Blender 5.2 headless export and FBX re-import verification completed successfully:

- Dimensions: 263 × 146 × 142 cm.
- Geometry: 2,510 triangles, four material slots, one UV layer.
- Collision: five box colliders, unchanged from V7.
- Verifier: `PROBLEMS []`; minimum Z approximately -0.032 cm.
- V9 source checks pass, including front-plane and side-clearance constraints.

The generated FBX, manifest, verifier log, and previews are local under the gitignored
`Saved/CaptiveSky/ComfyBlender/RatHollow_HiddenV9/` directory. The entrance and
three-quarter previews are Blender structural checks, not Unreal screenshots. The
entrance opening is visible and the added roots no longer project beyond the log's
front silhouette, though the bark remains strongly striated in the preview.

## Still required before placement

Review a disposable Unreal import for material appearance, ground contact, entrance
readability at Rat scale, collision, navigation, and Nanite/foliage compatibility.
No live map, object placement, character memory, or `wildlife_homes` data was changed
for this candidate.
