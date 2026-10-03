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

## Visual screening follow-up (2026-10-03)

An isolated UE 5.8.3 scene-capture test placed `Festuca_gautieri_LD`, `Phalaris_arundinacea_LD`, and `Typha_latifolia_LD` beside a known-good engine cube. The cube and floor render; Unreal loads all three meshes, reports centered bounds (local origins within 12 cm of zero), and completes their Nanite builds before capture. The plants nevertheless remain invisible in the captured scene. The result persisted with the source materials, with Nanite disabled, and with the mesh yaw changed; this does **not** establish that the assets are visually sound or broken. The scratch renderer required UE's `-DDC-ForceMemoryCache` and `-shaderworkingdir` options because the normal user-level cache/worker paths are outside the writable workspace. All experiment files and captures are under ignored `Saved/CompileScratch/`; no primary `Content/` files were changed.

Do not import these candidates on the strength of the earlier asset-load probe alone. Next diagnose them in the Static Mesh Editor/asset-thumbnail path (or with a small viewport test in the owning project), inspect the material and mesh render settings, then repeat a gameplay-scale 30 FPS check only if the plants can first be viewed convincingly.

## Existing foliage baseline (2026-10-03)

The connected main UE 5.8.3 editor successfully generated asset thumbnails for all nine vegetation meshes already in CaptiveSky. This confirms the existing assets display in their normal asset-preview path; it does not resolve the separate scratch scene-capture issue above. Static Mesh Tools reports these LOD0 triangle counts, LOD counts (including LOD0), and Nanite settings:

| Existing mesh | LOD0 triangles | LODs | Nanite |
| --- | ---: | ---: | --- |
| `grass_01_02_mesh` | 774 | 4 | enabled |
| `grass_01_03_mesh` | 1,200 | 4 | enabled |
| `grass_01_04_mesh` | 422 | 4 | enabled |
| `ground_05_01` | 208 | 4 | enabled |
| `ground_01_01` | 46 | 4 | enabled |
| `ground_01_02` | 46 | 4 | enabled |
| `spruce_half_01` | 6,738 | 5 | enabled |
| `Rhododendron__Everestianum__HD` | 1,358 | 1 | enabled |
| `Typha_latifolia_LD` | 1,403 | 1 | enabled |

The six meadow/ground-cover meshes are exceptionally light and already have four LODs. Use them as the budget baseline when evaluating additions; the PlantFactory candidates have much denser Nanite builds, so any use needs an explicit in-world 30 FPS check rather than assuming their `LD` label makes them equivalent. The existing set currently provides three grass forms and three low ground plants alongside spruce, rhododendron, and cattail; the 278-mesh neighboring collection remains a source pool, not 278 verified species.
