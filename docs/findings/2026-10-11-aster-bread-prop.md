# Aster bread practice prop

## Intent

The worldbuilding milestone calls for a simple practice bread set that can
eventually give Aster's craft tradition a visible, ordinary-life expression.
This asset is deliberately only a serving vignette: one oval hearth loaf with
three fine baked scores, a turned cut slice with visible crumb pores, a worn
board, folded linen, and crumbs.
It does not define or seed a recipe, alter either resident's memory, or place
anything in the Island.

## Source and export check

`Scripts/AssetPipeline/make_bread_texture.py` generates a deterministic,
tileable 512px crust albedo, DirectX normal, and non-metallic ORM set using
periodic harmonics and wrapped crumb pores; it needs only NumPy and Pillow.
The generated maps are local under `Saved/CaptiveSky/ComfyBlender/Library/Textures/`;
the albedo was visually checked for broad mottling rather than directional
grain. `Scripts/AssetPipeline/assets/bread.py` builds a compact sub-metre prop
from that crust, a dedicated warm crumb material, and the shared wood and canvas
textures. The first exported preview prompted a warmer cut face, thinner flour
scores, and a visible foreground linen fold. Collision is limited to the board;
bread, crumbs, and cloth stay non-blocking. The standalone source-contract suite
passes all four tests.

The normal `make.py bread` pipeline built, exported, re-imported, and rendered
`Saved/CaptiveSky/ComfyBlender/BreadPracticeSet/Preview.png`. Verification reports
54 × 36 × 18 cm bounds, origin/ground contact at `ZMIN 0.0`, about 1,014
triangles, one `UVMap`, six resolved material slots, one board box collider,
`PROBLEMS []`, and `VERIFY DONE`. Review logs are in
`Saved/CaptiveSky/ComfyBlender/_work/bread-review-20261011/logs/`. The export is
local-only and remains unplaced. It is also structurally imported to
`/Game/Generated/ComfyBlender/BreadPracticeSet/SM_BreadPracticeSet_01`; Unreal
reported matching 54 × 36 × 17.9 cm bounds, all six material assignments, one
embedded box collider, and `validation_errors: []` in
`Saved/CaptiveSky/ComfyBlender/BreadPracticeSet/unreal_import.json`. Neither the
imported materials nor collision has yet been visually inspected in the live
editor; no Island map state was touched.

## Visual refinement trial (2026-10-12)

The first preview exposed three source-level problems: the small board sampled
too many dark planks from the shared wood atlas, raised pale score tubes looked
like wires, and the cut face was a blank disk. The source now samples a broader
wood region, uses fine dark baked scores, adds fourteen low-profile toasted
crumb pores, warms the cut crumb, and gives the linen a shallow drape. The shared
wood/canvas maps and generic `lib.py` were not changed.

The revised FBX was built and re-imported by Blender 5.2.2 in an isolated output
root, with **1,460 triangles**, bounds 54 × 36 × 17.8 cm, one UV channel, seven
resolved material slots, one board collider, `ZMIN 0.0`, and `PROBLEMS []`.
The verified trial preview is
`Saved/CaptiveSky/ComfyBlender/BreadPracticeTrial_20261012/BreadPracticeSet/Preview.png`.
This is still only an offline FBX preview: the already-imported Unreal asset,
Inn placement, and Island map were not changed, and the revised material has
not been checked under Island lighting.

## Remaining

Before using it in the world, inspect the revised FBX in the editor and check
the warm crust/crumb contrast, board grain, linen folds, tangents, and scale under
Island lighting. Decide separately whether it belongs on an inn table. No recipe, Aster
memory, request approval, or Island map state is seeded by this asset.
