# RatHollow Hidden V8 candidate

V8 is a reversible source-and-preview iteration on the hidden fallen-wood home.
It is not imported into Unreal or placed in the Island. V7 remains the comparison
baseline and is documented in [the V7 review](2026-10-31-rathollow-hidden-v7.md).

## Changes and export

The scripted Blender source, `Scripts/AssetPipeline/assets/rathollow_hidden_v8.py`,
inherits V7's trunk, materials, five box colliders and named entrance/nest anchors.
It adds two asymmetrical, ground-hugging root shoulders around the entrance. The
opening remains unobstructed in the front preview; the extra geometry is decorative
and adds no collision.

Blender 5.2 headless export and FBX re-import verification completed successfully:

- Dimensions: 274 × 151 × 142 cm.
- Geometry: 2,684 triangles, four material slots, one UV layer.
- Collision: five box colliders, unchanged from V7.
- Anchors: `entrance` at `[-132, 0, 66]` cm and `nest_bed` at `[-32, 0, 32]` cm.
- Verifier: `PROBLEMS []`; minimum Z is approximately -0.032 cm.
- Script checks: V7 and V8 regression tests pass; triangulation is applied to export meshes.

Generated FBX, manifest, verifier log and previews live under the gitignored
`Saved/CaptiveSky/ComfyBlender/RatHollow_HiddenV8/` directory. The front and oblique
previews are Blender export previews, not Unreal screenshots. The entrance reads
open from the front and the side roots modestly break up the round rim in the oblique
view; one root tip projects beyond the trunk, so this is a study rather than a final
art pass.

## Still required before placement

Review a disposable import in Unreal for material appearance, ground contact,
entrance readability at Rat scale, collision and navigation. No live map, object
placement, character memory, or `wildlife_homes` data was changed for this candidate.
