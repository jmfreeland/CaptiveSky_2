# Woodland strawberry groundcover candidate — 2026-10-13

## Why

The existing local preview was a very small, sparse specimen: 78 × 63 cm, eight
compound leaf fans, three flowers, two berries, and about 2,038 triangles. Its
leaflets read as angular and the clump was too slight to help structure the
woodland floor. The Island still needs selective understory layers rather than
more blanket foliage.

## Source revision and local result

`Scripts/AssetPipeline/assets/woodland_wildstrawberry.py` now builds four
staggered rosettes with 15 trifoliate fans, five flowers, and four berries. The
leaflets are broader, have more intermediate contour rows and shallower edge
teeth, and use a modestly lighter green. The plant remains low and asymmetric;
it has no collision so it cannot snag a walking resident.

Built with Blender 5.2.2 into the isolated ignored export folder
`Saved/CaptiveSky/ComfyBlender/Codex_WoodlandStrawberryPatch_20261013/`.
The exported FBX manifest reports 104 × 93 × 22 cm, 4,840 triangles and five
material slots. The verifier re-imported the FBX with the same dimensions,
UVMap, no collision meshes, and `PROBLEMS []`. The new render is visibly broader
and fuller than the earlier sparse preview, while retaining the pipeline's
faceted low-poly style.

## Limits and next check

This is an unplaced art candidate, not proof of an Island improvement. At 4,840
triangles it is suitable to assess as a distinct clump, but should not be
blanket-scattered without measuring instance count, frame time and texture cost;
the asset pipeline notes that its current exports are not tuned for LODs. No
Unreal Editor process was available during this review, so the FBX has not been
imported, lit against the actual forest floor, or reviewed at gameplay distance.
Next, once the editor can be opened reliably, import it as an isolated candidate
and inspect the foliage-to-litter transition in a bounded real-RHI Island view.
Keep it out of the authored map until silhouette, ground contact, material
contrast and performance are judged in context.
