# Rat hollow weathered V4 candidate

## Result

Created a separate asset-pipeline recipe for the Rat's hidden fallen-wood
hollow. It preserves the validated V3 entrance and bark UV scale, makes the
inner wood warmer, and adds five broader overlapping moss patches along the
trunk crown. V2 and V3 remain unchanged.

The V4 FBX is 192 x 118 x 89 cm with 1,324 triangles, five material slots,
eight embedded box colliders, and the same entrance-center/threshold anchors
as V3. Blender 5.2.2 exported and re-imported the FBX successfully; the
verifier reported `PROBLEMS []`. Sixteen focused V2/V3/V4 tests pass.

## Visual review and limits

The isolated front-left preview clearly shows the ground-level entrance and
the straw bed, with a more connected moss crown than V3. The bark still reads
as strongly banded, stylized weathered wood; this is not yet a naturalistic
fallen root. Review the preview at
`Saved/CaptiveSky/ComfyBlender/RatHollowRootArchBurrowWeatheredV4/Preview.png`.

The output is local and gitignored. It has **not** been imported into Unreal,
placed in the Island, or checked for Rat traversal, concealment, lighting, or
collision against the live world. No map or existing generated asset was
modified. Treat V4 as a review candidate, not a selected or approved home.
