# Megaplant readiness in the live project

## Verified in the open Unreal editor

- `ProceduralVegetationEditor` is present and enabled in the project. No plugin toggle or editor restart is currently needed to use PVE.
- The project asset registry contains installed content under `/Game/Megaplant_Library/` for four species: Common Hazel, European Aspen, European Beech, and Norway Maple.
- European Beech includes both `/Game/Megaplant_Library/Tree_European_Beech/Tree_European_Beech_01/PVE_European_Beech_01` (class `ProceduralVegetation`) and `PVE_Preset_European_Beech_01` (class `ProceduralVegetationGrowerPreset`), as well as baked skeletal-mesh outputs.
- A representative baked output, `Tree_European_Beech_01_A`, is a SkeletalMesh with 1,113 bones, 5,985 vertices, 2,193 registry triangles, one LOD, and Nanite enabled. The registry also reports 22,157,107 Nanite triangles; this is metadata, not an in-world performance measurement, but it is enough reason to benchmark one tree before scattering many.

## Recommended first validation

Do not begin with a broad PCG scatter. First inspect one Beech output for appearance and material correctness, then place one instance in a reversible test area and measure frame time and memory in a short, bounded play session. Compare with the existing landscape/foliage baseline before choosing a PVE export, static/non-Nanite representation, or instancing strategy. Do not alter the authored Island landscape material or save a large scatter without a measured result.

The Editor MCP inspection was read-only: no Megaplant assets were loaded, modified, placed, or saved, and the Island map was not changed. The asset's source PVE graph compatibility and runtime rendering remain unverified.
