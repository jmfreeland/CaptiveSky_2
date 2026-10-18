# Tideglass marsh-marigold candidate (2026-10-18)

## Ecological fit

Marsh-marigold (*Caltha palustris*) grows at freshwater margins, in wet woods,
and in seasonally flooded meadows; it does not tolerate permanently flooded
ground. Its broad rounded foliage and glossy yellow sepals are an intentional
silhouette and color contrast to the existing sword-leaved yellow-flag iris,
fine soft rush, and low trifoliate bogbean candidates. This is a seasonal
vegetation candidate for a damp, fluctuating water edge, not an underwater or
permanently submerged plant. Sources: [BSBI Fermanagh species account](https://fermanagh.bsbi.org/caltha-palustris-l)
and [Kew Plants of the World Online](https://powo.science.kew.org/taxon/urn%3Alsid%3Aipni.org%3Anames%3A42867-2).

## Candidate design

`Scripts/AssetPipeline/assets/tideglass_marsh_marigold.py` creates four low
leaf rosettes with broad, gently cupped blades and five flower stalks carrying
six glossy sepals each. The geometry is deterministic, uses four plain
material groups, adds no textures or collision, and does not touch the Island
map or issue model requests. The silhouette is kept low and wide to bridge the
gap between groundcover and taller rushes.

## Export validation and review gate

The source-contract tests check the species form, material grouping,
determinism, shared mesh count, and collision-free construction (3 passed).
Blender 5.2.2 exported and re-imported the FBX with the CPU preview renderer:
79 x 81 x 38 cm, 1,968 triangles, four material slots, one `UVMap`, no
collision meshes, 0.92 cm ground clearance, and `PROBLEMS []`. The isolated
preview shows the broad leaf/bright bloom contrast and uncomplicated
low-poly silhouette; it cannot establish Unreal material response, waterline
fit, readability at play distance, or frame cost.

Keep the asset out of the saved Island map. Once Unreal is responsive, export
and verify the FBX, import it into a fresh isolated
`/Game/Generated/ComfyBlender/TideglassMarshMarigold/` folder, and inspect it
at the fluctuating waterline under representative daylight. Only propose a
disposable placement trial if the leaves read as broad marigold foliage, the
yellow sepals remain visible at play distance, and a bounded matched capture
shows acceptable frame cost without increasing texture pressure.
