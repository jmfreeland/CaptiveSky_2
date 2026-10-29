# Rat hollow readable-entrance V3 candidate

## Result

Built a separate, local-only V3 candidate for the Rat's chosen hidden fallen-wood hollow. It keeps the V2 structure intact and enlarges the ground-level entrance arch while using a warm, non-emissive hollowwood color to make the interior easier to read.

The validated FBX is `SM_RatHollowRootArchBurrowLightwellV3_01.fbx`, approximately 192 x 118 x 89 cm and 924 triangles, with one UV channel, five material slots, eight simple collision boxes, and entrance center/threshold anchors in the manifest. The exported FBX re-imported successfully in Blender and the pipeline reported `PROBLEMS []`.

## Review

The corrected preview shows the entrance from the front-left, with the lower opening and straw nest visible. This is a clearer candidate than the dark, nearly closed-looking V2 preview, but it is still a stylized log-stump shelter rather than a naturally irregular fallen root. Treat it as a visual candidate for review, not as a final approved home.

Preview: `Saved/CaptiveSky/ComfyBlender/Codex_RatHollowV3_20261029/RatHollowRootArchBurrowLightwellV3/Preview.png` (local generated output; ignored by Git).

## Scope and verification

- Source recipe: `Scripts/AssetPipeline/assets/rat_hollow_rootarch_v3.py`; V2 source was not modified.
- Targeted V3 tests: 5 passed. Full asset-pipeline suite: 106 passed.
- Blender export and FBX re-import/preview verification passed; validation was run CPU-only in an isolated output/work directory.
- The isolated output uses copies of the existing local Bark, Moss, and Straw texture maps. It does not modify the shared library.
- Nothing was imported into Unreal or placed in the Island. No map or `Content/` files were changed.
