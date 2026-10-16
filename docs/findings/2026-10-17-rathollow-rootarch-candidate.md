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
1.868 × 1.177 × 0.886 m and 936 triangles / 565 polygons, with a UVMap and
Bark/Hollowwood/Moss/Dirt/Straw material slots. Its mouth is about 55 cm across.
The verifier reported `PROBLEMS []` and a minimum Z of 1.3 cm. It has no
collision, so it is not ready for navigation or Rat traversal. A source audit
traced the FBX tangent-space warning to the six- and nine-sided tube end caps.
The generator now triangulates only those cap faces before export; shell quads
and their UVs remain intact. The latest export emitted no tangent-space warning;
re-import found no face larger than a quad and retained the `UVMap`. Unreal-side
tangent handling has not been checked.

## Runtime integration constraint

`AIslandWildlifeHome` currently places the original RatHollow mesh at a fixed
`(+117, 0, 0) cm` component offset from the recorded entrance anchor
(`IslandWildlifeHome.cpp` / `IslandWildlifeHome.h`). This candidate's opening
center is approximately `(-78, -1, 47) cm` in mesh-local coordinates, with an
inner radius of 27.5 cm. Reusing the existing offset would put the opening
center about 39 cm forward and 47 cm above the actor anchor; its lower edge
would still sit roughly 19.5 cm above it. Do not swap the asset path under the
old transform. Before integration, confirm whether `EntranceLocation` denotes
the opening center or the ground threshold, then give this mesh its own
component offset and verify Rat approach/navigation in the disposable map.
The persisted home record and its anchor remain unchanged.

The current three-quarter preview is
`Saved/CaptiveSky/ComfyBlender/RatHollowRootArch/Preview.png` (generated,
gitignored output). It was rendered in Blender 5.2.2 with Cycles CPU, 12 samples,
and two worker threads while leaving the unresponsive Unreal editor untouched.
It shows the bark maps and entrance bedding. No Unreal import or map change was
made. Gameplay, Rat-scale clearance, collision, in-world bark scale, and
human-height concealment remain unverified.

## Next step

Keep the canonical Rat home and saved Island unchanged. When the editor is
available, import the candidate into a disposable Island copy and review the
entrance at human and Rat heights with woodland cover; check imported tangents
and decide whether a simple collision shell is needed. A Tripo-generated source
may be worth trying, but its CLI could not be fetched from npm in this session
(`ENOTFOUND`); no generation request or credit spend was made. Any paid attempt
should be planned and confirmed before submission.
