# Tideglass bogbean candidate (2026-10-18)

## Ecological fit

Bogbean (*Menyanthes trifoliata*) is a wetland perennial suited to shallow
lake, pool, and ditch margins; local BSBI describes it as an emergent or
semi-aquatic plant that can form marginal stands or floating mats. That makes
it a useful silhouette contrast to the upright Typha and Phalaris already
reviewed at Tideglass: broad, low trifoliate leaf groups beneath pale,
fringed flowers on several short scapes. See the
[BSBI Fermanagh species account](https://fermanagh.bsbi.org/menyanthes-trifoliata-l)
and [Kew Plants of the World Online](https://powo.science.kew.org/taxon/urn:lsid:ipni.org:names:50970102-1).

## Candidate and validation

`Scripts/AssetPipeline/assets/tideglass_bogbean.py` creates a deterministic,
collisionless clump using simple leaf, petiole, pale flower, blush, and fringe
materials. It uses no generated texture or runtime system and adds no model
requests. The asset pipeline export is kept local at
`Saved/CaptiveSky/ComfyBlender/TideglassBogbean/`.

The 74-test asset-pipeline suite passed. Blender 5.2.2 rebuilt, re-imported,
and rendered the FBX on CPU: 41 x 39 x 44 cm, 5,360 triangles, seven material
slots, no collision, `ZMIN 0.0161`, and `PROBLEMS []`. The original preview
demonstrates the mesh and material separation only; it does not show the asset
in water or at its likely game-view scale.

## Raceme art pass (2026-10-19)

The first preview made all 13 flowers read as almost uniformly spaced,
same-sized discs. The source now applies deterministic per-flower spacing,
orientation, and scale variation while keeping the same three stalks and
five-lobed fringed flower construction. Blender 5.2.2 rebuilt and re-imported
the export: 41 x 39 x 44 cm, 5,360 triangles, seven slots, no collision,
`ZMIN 0.0161`, and `PROBLEMS []`; the three focused source-contract tests pass.
The updated preview shows a small, not dramatic, reduction in regularity. This
is still an offline shape review: Unreal material response, waterline fit,
game-view readability, and in-world frame cost remain unverified. Keep it
unplaced until the isolated Unreal review can be completed.

This remains a review candidate, not an Island placement. Blender's exported
FBX/preview check establishes mesh/export structure only. Import into a
disposable Unreal copy before deciding whether the flat-material palette,
flower scale, waterline relation, and silhouette read naturally beside the
existing Tideglass species. Do not add it to the saved Island map until a
real-RHI Game review confirms scale and frame cost in the actual wet-edge view.
