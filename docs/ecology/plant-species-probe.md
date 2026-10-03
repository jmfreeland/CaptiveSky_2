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

An isolated UE 5.8.3 scene-capture test placed `Festuca_gautieri_LD`, `Phalaris_arundinacea_LD`, and `Typha_latifolia_LD` beside a known-good engine cube. The cube and floor render; Unreal loads all three meshes, reports centered bounds (local origins within 12 cm of zero), and completes their Nanite builds before capture. The plants nevertheless remain invisible in this **SceneCapture2D** image. The result persisted with the source materials, with Nanite disabled, and with the mesh yaw changed; this does **not** establish that the assets are visually sound or broken. The scratch renderer required UE's `-DDC-ForceMemoryCache` and `-shaderworkingdir` options because the normal user-level cache/worker paths are outside the writable workspace. All experiment files and captures are under ignored `Saved/CompileScratch/`; no primary `Content/` files were changed.

Follow-up with the actual Level Editor viewport confirmed that `Festuca_gautieri_LD` and `Phalaris_arundinacea_LD` geometry does render; `Typha_latifolia_LD` was included as a visual reference. The 1600×1000 viewport capture is at `Saved/CompileScratch/Codex_SpeciesProbe_20261003/Project/Saved/SpeciesThumbnails/SpeciesViewport.png`. The capture is monochrome and strongly overexposed, with long shadows and a test grid, so it proves visibility in the viewport but is **not** a trustworthy judgment of source-material appearance or suitability in the island. The contrast with the blank SceneCapture2D result makes that earlier failure specific to the test capture path, not evidence that the plant meshes themselves are invisible. No candidate has been imported into CaptiveSky's primary `Content/`.

Before recommending either candidate, make a better-lit isolated preview that preserves material colors and inspect mesh/material dependencies. If the appearance holds up, migrate only approved assets and dependencies, substitute them within the current foliage instance budget, and run the gameplay-scale 30 FPS check. Do not import candidates into the primary project until approved.

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

## Flower collection source probe (2026-10-03)

The neighboring collection's 96 `flowerMesh` packages are arranged as 20 numbered groups, with multiple variants in many groups. In the isolated UE 5.8.3 project, the first mesh of each group loads as a Static Mesh and resolves its per-mesh material instance to the shared `MA_Flowers` parent. The associated 8192×8192 `Combined_Flowers_A` color atlas and `Combined_Flowers_O` opacity atlas export successfully; visual inspection of those atlases confirms a broad range of flower-head, leaf, and stem shapes and colors (including daisy-like, poppy-like, yellow and purple forms). This is atlas evidence only, not botanical identification or proof of the rendered look.

The source `MA_Flowers` depends on collection-specific material functions and parameters. For a diagnostic only, a simple UE masked/opaque atlas material was built in the scratch project and assigned to the sample actors. The capture shows its engine cube and floor, but still no flower meshes. Therefore this preview does **not** validate the meshes' apparent size, shape, or material/UV mapping. Suspect a sample-scene transform/visibility or mesh rendering issue; do not choose or migrate flower assets based on the atlas alone. A proper per-asset viewport thumbnail or corrected isolated preview is still required. All copied collection assets, test materials, exports, and captures remain under ignored `Saved/CompileScratch/`; no primary `Content/` files were changed.

The most promising next triage is to open representative assets in UE's Static Mesh Editor/asset-preview path, which has already worked for CaptiveSky's existing plants, and identify one or two clearly contrasting flower forms. If chosen and approved, migrate only those mesh/material/texture dependencies, replace some existing low ground-cover instances (not increase the instance budget), and verify visual placement plus the established gameplay-scale 30 FPS floor.
