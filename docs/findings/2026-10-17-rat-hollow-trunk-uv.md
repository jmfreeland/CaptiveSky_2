# Rat hollow trunk-axis bark UV trial — 2026-10-17

## Change

The `RatHollowRootArchBurrow` source used box projection on a curved trunk whose
long axis is local X. The reviewed source export showed inconsistent bark
orientation around the shell. Its outer and inner shells now use cylindrical
UVs around local X, with planar UVs retained on the cut ends and entrance lip.
Root branches keep their existing mappings. This changes only the source-built
candidate; it does not change the saved Island, imported Unreal assets, or Rat
home record.

## Validation

`test_rat_hollow_rootarch_v2.py` passes 5/5 checks. Blender 5.2.2 rebuilt and
re-imported the FBX with `PROBLEMS []`: 192 × 118 × 89 cm, approximately 924
triangles, one UV channel, five material slots, and eight simple collision
boxes. The entrance-facing local preview is at
`Saved/CaptiveSky/ComfyBlender/Codex_RatHollowTrunkUVTrial_20261017/RatHollowRootArchBurrow/Preview.png`.

## Limits and next check

The preview suggests a more consistent longitudinal grain on the trunk shell,
but it is a studio render, not evidence of acceptable Unreal material import,
Island lighting, ground integration, concealment, or Rat traversal. The
candidate has not been imported or placed. When the project can be opened
reliably, compare it side-by-side with the current trial asset in the same
Island lighting and inspect both the Rat-height entrance and human-height
approach. Keep the canonical home and Island map unchanged until that review
and a capsule traversal pass both succeed.
