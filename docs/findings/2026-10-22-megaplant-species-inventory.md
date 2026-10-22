# Installed Megaplant tree inventory: wet-edge candidate (2026-10-22)

## Evidence

A read-only Asset Registry search of `/Game/Megaplant_Library` found six tree species already installed: Norway Maple, European Aspen, European Beech, Black Alder, Common Hazel, and Aleppo Pine. The current Island runtime integrates the first three as broadleaf tree instances; hazel appears separately as coppice. Black Alder and Aleppo Pine are not currently part of that runtime mix.

The imported trees are skeletal meshes backed by Procedural Vegetation assemblies and DynamicWind. Representative asset tags report:

| Variant | Triangles | Vertices | Bones | LODs |
|---|---:|---:|---:|---:|
| Black Alder A | 2,940 | 8,820 | 3,726 | 1 |
| Aleppo Pine A | 4,128 | 12,384 | 2,688 | 1 |
| Norway Maple A | 53,744 | 28,004 | 462 | 1 |

The Black Alder and Aleppo Pine bark and foliage material instances both use the ProceduralVegetation plugin's `MA_Foliage_Trees` parent, as do the current Maple/Aspen/Beech materials. The live editor has logged missing material-usage warnings for the current broadleaf meshes, so the new instances should be presumed to need the same compatibility fix until proven otherwise. This is a risk inference from their shared parent, not a rendered test.

## Decision and gate

Black Alder is the most promising next species to evaluate at Tideglass's wetland edge; it could add a visibly distinct canopy without a global density increase. Its much larger bone count, single LOD, and unverified instanced material compatibility make an immediate Island scatter unsafe while the project is already near its video-memory budget. Aleppo Pine is retained as a secondary option, not a default addition.

Before integration, preview one assembly in isolation with the project's instanced-skinned foliage path. Verify bark and leaf materials, ref-pose and wind appearance, useful mid-distance silhouette, and incremental frame/VRAM cost. Route its material instances through the validated foliage-material workflow. Do not place either species in the main Island or increase the existing vegetation budget on the strength of this inventory alone.

## Scope

This was an Asset Registry/material-parent audit only. No content asset, map, tree scatter, or runtime state was changed, and no PIE session was started.
