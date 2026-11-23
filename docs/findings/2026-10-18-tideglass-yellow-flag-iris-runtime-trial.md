# Tideglass yellow-flag iris runtime trial

## Change

Added the already-imported, local yellow-flag iris mesh as a specialist species
for two of the eight existing Tideglass pollinator pockets. The deterministic
seed-rotated pattern now assigns three pockets to the lower marsh-marigold,
two to the taller broad-leaf iris, and leaves three with the established meadow
flower mix. All members of a pocket retain their existing species coherence.

The trial reuses the existing eight anchor sites and three-instances-per-pocket
ceiling; it adds no candidate traces, density, textures, collision, navigation,
or saved-map actors. The imported iris is a 7,500-triangle, 0.12 MB mesh with
five plain-color materials and no collision, per its isolated Unreal import
review in `2026-10-18-yellow-flag-iris-candidate.md`. If the asset is missing,
the site falls back to the pre-existing meadow mix.

## Verification and limits

The focused ecology test checks the imported mesh binding, no-collision and
navigation settings, deterministic seed rotation, and the 3/2/3 pocket split.
The broader ground-cover test also checks the added HISM against the existing
rendering contract.

This is a reversible runtime source trial, not approval to save placements into
the Island map. The imported mesh has only one LOD, and its Island daylight,
reverse-angle silhouette, gameplay-distance readability, and the 30-FPS p95
play safeguard have not been checked in a live editor/gameplay view. The desktop
bridge currently does not expose the user's responsive Unreal Editor, so do not
claim those visual/performance gates passed. Revisit the two-pocket placement
after a live in-world view and bounded zero-model-request performance capture.
