# Aster bread practice prop

## Intent

The worldbuilding milestone calls for a simple practice bread set that can
eventually give Aster's craft tradition a visible, ordinary-life expression.
This asset is deliberately only a serving vignette: one oval hearth loaf with
three flour scores, a turned cut slice, a worn board, folded linen, and crumbs.
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

## Remaining

Before using it in the world, inspect the imported static mesh in the editor and
check the warm crust/crumb contrast, woven linen, tangents, and scale under Island
lighting. Decide separately whether it belongs on an inn table. No recipe, Aster
memory, request approval, or Island map state is seeded by this asset.
