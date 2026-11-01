# Woodland dog-violet understory candidate — 2026-11-01

`WoodlandDogViolet` is a small, collisionless spring-flower study for a shaded
woodland edge. It adds a distinct purple flower and heart-shaped ground-leaf
silhouette alongside the existing green fern, bluebells, white ramsons, and
red-cap fungi. The [Woodland Trust species profile](https://www.woodlandtrust.org.uk/trees-woods-and-wildlife/plants/wild-flowers/common-dog-violet/)
describes common dog-violet as a UK-native woodland/grassland/heathland plant,
flowering April–June and growing up to 15 cm. Kew accepts *Viola riviniana*
with a European native range ([Plants of the World Online](https://powo.science.kew.org/taxon/urn:lsid:ipni.org:names:869126-1)).
This supports the visual/habitat concept, not an exact placement on the Island.

## Candidate build

- Four overlapping low rosettes with heart-like leaves and five modest purple
  blooms; maximum authored bloom height is 15 cm after export scaling.
- Blender 5.2.2 FBX re-import reports `PROBLEMS []`: 51 × 47 × 13 cm,
  3,390 triangles, four material slots, one `UVMap`, and no collision shapes.
- Preview: `Saved/CaptiveSky/ComfyBlender/WoodlandDogViolet/Preview.png`.
- Source: `Scripts/AssetPipeline/assets/woodland_dog_violet.py`; source-contract
  tests: `Scripts/AssetPipeline/tests/test_woodland_dog_violet.py`.

The simple Blender preview reads as a low violet patch but still has angular,
stylized leaves and thin flower stems. This is a candidate for a sparse,
shaded edge pocket—not a high-density scatter asset. No Unreal import,
Island placement, seasonal visibility, gameplay-distance readability, or
runtime performance has been validated. Review it in a disposable Island copy
beside the existing fern and bluebell forms before considering use; keep it
unplaced if its leaf planes or stems look artificial under Island lighting.
