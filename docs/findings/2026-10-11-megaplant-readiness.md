# Megaplant readiness in the live project

## Verified in the open Unreal editor

- `ProceduralVegetationEditor` is present and enabled in the project. No plugin toggle or editor restart is currently needed to use PVE.
- The separate `r.Nanite.Foliage` renderer CVar is `0` in the live editor (`SearchCVars`) and is explicitly set to `False` at `Config/DefaultEngine.ini:188`. Fab's Megaplant setup notes ask users to enable **Nanite Foliage (Experimental)** for its Nanite foliage workflow ([example: European Beech](https://www.fab.com/listings/cefe5722-9c31-4aa2-9ee2-e5426610d5e6)). This is distinct from enabling the PVE editor plugin.
- The project asset registry contains installed content under `/Game/Megaplant_Library/` for four species: Common Hazel, European Aspen, European Beech, and Norway Maple.
- European Beech includes both `/Game/Megaplant_Library/Tree_European_Beech/Tree_European_Beech_01/PVE_European_Beech_01` (class `ProceduralVegetation`) and `PVE_Preset_European_Beech_01` (class `ProceduralVegetationGrowerPreset`), as well as baked skeletal-mesh outputs.
- Common Hazel also contains a PVE graph with branch and leaf palette dependencies. Its output variants A-D are SkeletalMeshes. Registry metadata for the smallest, D, reports 11,429 triangles and 83 bones; C has 32,627 triangles and 142 bones.
- Read-only thumbnails of Hazel variants C and D show woody branch structures without visible leaves, while the separate `Leaf_Common_Hazel_01` asset renders as a green leaf. A leaf-bearing in-world result therefore needs the graph's full foliage/export path; simply placing the small SkeletalMesh output has not been shown to produce a finished tree.
- A representative Beech output, `Tree_European_Beech_01_A`, is a SkeletalMesh with 1,113 bones, 5,985 vertices, 2,193 registry triangles, one LOD, and Nanite enabled. The registry also reports 22,157,107 Nanite triangles; this is metadata, not an in-world performance measurement, but it is enough reason to benchmark one tree before scattering many.

## Recommended first validation

Do not begin with a broad PCG scatter. First inspect the Hazel PVE graph and confirm its export produces the leaf-bearing result. Epic's [UE 5.8 PVE guide](https://dev.epicgames.com/documentation/unreal-engine/procedural-vegetation-editor-pve-in-unreal-engine) says the Export node bakes graph output into renderable assets and that only exported assets should be placed in the world. Then compare a single exported plant with Nanite Foliage off/on in a reversible test area and measure frame time and memory in a short, bounded play session. Compare with the existing landscape/foliage baseline before choosing a PVE export, static/non-Nanite representation, or instancing strategy. Do not alter the authored Island landscape material or save a large scatter without a measured result.

The Editor MCP inspection was read-only: no Megaplant assets were loaded, modified, placed, or saved, and the Island map was not changed. The asset's source PVE graph compatibility and runtime rendering remain unverified.
