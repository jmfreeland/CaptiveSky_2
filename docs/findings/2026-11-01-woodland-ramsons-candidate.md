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
  florets per umbel. The flower stems vary from 38–43 cm, within the up-to-45 cm
  identification range in [Plantlife's woodland plants guide](https://www.plantlife.org.uk/wp-content/uploads/2024/06/WEB-Plantlife-ID-Guide.pdf).
- Refined export: 98 × 101 × 49 cm, 4,222 triangles, five plain material slots,
  one UV layer, no collision shapes.
- Blender 5.2.2 FBX re-import: `PROBLEMS []`.
- Blender 5.2.2 default-EEVEE FBX preview. The refreshed export is local under
  the gitignored `Saved/CompileScratch/Codex_RamsonsClumpRefine_20261101/` root;
  the original export remains in its separate scratch folder.
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

The original preview confirmed the individual leaf shapes but read as seven
separate fans, and its 23.5 cm stems let the white umbels disappear in the
framing. The revised source tightens the rosettes, narrows the leaves, and raises
the three scapes to 38–43 cm while preserving the 4,222-triangle budget. The new
preview and FBX re-import confirm a compact overlapping clump, clearer flower
heads, one UV layer, five material slots, no collision shapes, and
`PROBLEMS []`. The leaves still read somewhat planar in the plain-material
preview. This is a stronger art candidate, not placement approval: Unreal
lighting, ground contact, and visitor-scale readability remain unverified.
