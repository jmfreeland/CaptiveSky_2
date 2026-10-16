# RatHollow root-arch candidate — 2026-10-17

## Decision

The first root-arch pass was rejected: its rendered silhouette read as a raised
timber beam with supports. It has since been replaced by a tapered, open-mouth
hollow log with a narrowing dark interior, one-sided root fans, a grounded soil
bed, nest twigs, and small moss pads. The new preview reads as a hollow fallen
trunk, a meaningful improvement. The source uses the existing
`T_Lib_RatHollow_Bark` library maps from prior RatHollow prototypes, a warmer
custom inner-wood material, and a small straw bed visible inside the entrance.
Keep it as an offline candidate; do not place it on the Island yet. The bark
texture has strong striations that need a world-scale review.

## What was tested

`Scripts/AssetPipeline/assets/rat_hollow_rootarch.py` generates a visual-only
mesh through the local Blender pipeline. The latest FBX re-import measured
1.868 × 1.177 × 0.886 m and 936 triangles, with a UVMap and
Bark/Hollowwood/Moss/Dirt/Straw material slots. Its mouth is about 55 cm across.
The verifier reported `PROBLEMS []` and a minimum Z of 1.3 cm. It has no
collision, so it is not ready for navigation or Rat traversal. FBX export also
warned that an n-gon prevented tangent-space export; Unreal-side tangent handling
has not been checked.

The current three-quarter preview is
`Saved/CaptiveSky/ComfyBlender/RatHollowRootArch/Preview.png` (generated,
gitignored output). It was rendered in Blender 5.2.2 with Cycles CPU, 12 samples,
and two worker threads while leaving the unresponsive Unreal editor untouched.
The preview shows the bark maps and entrance bedding; no Unreal import or map
change was made. Gameplay, Rat-scale clearance, collision, in-world bark scale,
and human-height concealment remain unverified.

## Next step

Keep the canonical Rat home and saved Island unchanged. When the editor is
available, import the candidate into a disposable Island copy and review the
entrance at human and Rat heights with woodland cover; check imported tangents
and decide whether a simple collision shell is needed. A Tripo-generated source
may be worth trying, but its CLI could not be fetched from npm in this session
(`ENOTFOUND`); no generation request or credit spend was made. Any paid attempt
should be planned and confirmed before submission.
