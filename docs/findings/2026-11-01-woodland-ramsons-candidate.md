# Woodland ramsons candidate

`WoodlandRamsons` is a collisionless spring-understory study for a damp,
shaded deciduous-woodland pocket. The species choice adds a distinct broad-leaf
ground layer and a clear seasonal white-flower signal rather than another tree
or conifer-shaped silhouette. The Woodland Trust describes wild garlic as
forming spring woodland-floor carpets, with basal pointed leaves and rounded
clusters of small white flowers; it also notes its early value to pollinators
([Woodland Trust species profile](https://www.woodlandtrust.org.uk/trees-woods-and-wildlife/plants/wild-flowers/wild-garlic/)).
Kew accepts *Allium ursinum* as native from western and central Europe to the
Caucasus ([Plants of the World Online](https://powo.science.kew.org/taxon/529088-1)).

## Candidate build

- Seven paired basal rosettes, three leafless scapes, and sixteen small starry
  florets per umbel.
- Export: 119 × 123 × 32 cm, 4,222 triangles, five plain material slots, one
  UV layer, no collision shapes.
- Blender 5.2.2 FBX re-import: `PROBLEMS []`.
- Preview: CPU Cycles, eight samples. The grouped model is local under the
  gitignored `Saved/CompileScratch/Codex_WoodlandRamsons_20261101/` root.
- Source: `Scripts/AssetPipeline/assets/woodland_ramsons.py`; deterministic
  source tests live in `Scripts/AssetPipeline/tests/test_woodland_ramsons.py`.

## Review gate

This is a composed clump, not a low-cost mass-scatter mesh. Keep any first use
sparse and limited to a shaded, damp woodland floor; do not assign exact
placement from the species' general habitat alone. The Island has no seasonal
visibility behavior in this candidate, so leaves and flowers appear together
year-round if imported. Before placement, review silhouette, shading, ground
contact, and its 4,222-triangle cost in a disposable Unreal map and compare it
among the existing fern, strawberry, and litter forms. No Island instance,
map save, resident memory, or home record was changed for this candidate.

### Preview review (2026-11-01)

The exported-FBX preview confirms the individual leaf shapes and flower heads,
but the seven basal rosettes read as separate, rather flat fans at the current
framing; the three small white umbels are easy to lose against the ground and
sky. This is a useful first asset study, not yet a convincing woodland-floor
accent. Before any Unreal import, consider a tighter, more naturally overlapping
rosette composition and clearer flower-head silhouette, while keeping the
collisionless geometry light. A Blender preview cannot establish ground contact,
scale readability, or material response under Island lighting.
