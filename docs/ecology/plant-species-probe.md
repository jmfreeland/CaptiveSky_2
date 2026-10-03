# Plant species asset probe (2026-10-03)

CaptiveSky's asset registry currently has nine vegetation mesh assets: three grass meshes, three low ground plants, one spruce, one rhododendron, and one cattail. All nine are already referenced by the transient ground-cover system. The user's separate PlantFactory project (`EngineAssociation` 5.3) contains three additional mesh packages not present in CaptiveSky: *Phalaris arundinacea*, *Festuca gautieri*, and *Pinus ponderosa*.

To check compatibility without writing to CaptiveSky's ignored `Content/`, the PlantFactory `Content/Plants` folder was copied into a disposable blank UE 5.8.3 project under `Saved/CompileScratch/Codex_SpeciesProbe_20261003/Project`. A UE 5.8.3 Python asset-load probe confirmed:

- `Phalaris_arundinacea_LD`: loads as a Static Mesh; all five material slots resolve; bounds are approximately 135.5 × 130.9 × 77.4 cm; source package size is 1.13 MB.
- `Festuca_gautieri_LD`: loads as a Static Mesh; all six material slots resolve; bounds are approximately 61.6 × 58.2 × 57.6 cm; source package size is 2.72 MB.
- `Pinus_ponderosa_lone_HD`: the mesh package loads, but all seven material slots are null and the referenced material packages are absent from PlantFactory's `Content/Plants`. Its mesh package is 16.4 MB. Defer it unless the missing source materials can be recovered; do not ship it with broken/default materials.

The probe log is [here](../../Saved/CompileScratch/Codex_SpeciesProbe_20261003/Project/Saved/Logs/SpeciesProbe_Materials_UE58.log). This was an asset-load and material-reference check under `-NullRHI`, not a render, placement, cook, or performance test. No CaptiveSky map or primary-project asset was changed.

The useful next step, once approved, is a dependency-aware migration of only the two complete low-detail plants, then a species mix that replaces part of the existing meadow population rather than raising its instance budget. Festuca can be trialed in exposed meadow/rock transitions; Phalaris near wet edges. Validate both visually and through the existing 30 FPS gameplay-scale capture floor before keeping them.

## Additional foliage source inventory (2026-10-03)

The neighboring `CS_Interactive_2` project (`EngineAssociation` 5.6) contains 278 mesh-package files under `Content/PN_FoliageCollection/Meshes`: 96 in `flowerMesh` (12.9 MiB), 110 in `grassMesh` (18.0 MiB), and 72 in `groundPlantMesh` (8.3 MiB), 39.3 MiB total. This was a read-only filesystem inventory; those generic mesh groups have not yet been previewed, loaded in UE 5.8.3, or checked for complete material dependencies, and the counts are asset variants rather than confirmed distinct botanical species. Treat them as a visual-triage pool, not approved imports. A future pass should preview a small, distinctive subset in an isolated UE 5.8.3 project, verify mesh/material dependencies, then only migrate user-approved candidates. Any meadow trial should substitute species inside the existing instance budget and pass the 30 FPS gameplay-scale gate.
