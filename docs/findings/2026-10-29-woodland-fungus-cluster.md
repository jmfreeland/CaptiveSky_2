# Woodland fungus cluster candidate (2026-10-29)

## Intent

Add a small, shaded-woodland floor detail without introducing another heavy
foliage family or changing traversal. The procedural source builds five subtly
varied brown/gold mushrooms as a single low-poly, non-colliding static mesh.

## Validation

- Blender 5.2.2 scripted export and FBX re-import verifier completed with
  `PROBLEMS []`.
- Bounds: 34.3 × 24.0 × 21.7 cm; 960 approximate source triangles; one UV
  channel; three material slots; Z minimum 0.0035 m.
- Live UE 5.8 editor import at
  `/Game/Generated/ComfyBlender/WoodlandFungusCluster/SM_WoodlandFungusCluster_01`
  reports matching bounds, all three materials assigned, zero collision
  shapes, and `validation_errors: []`.
- The imported static-mesh editor reports 960 triangles and no collision
  primitives. Nanite is left disabled because this is a 960-triangle detail.
- Blender's exported-FBX preview and Unreal's static-mesh thumbnail both show
  the material breakup. Neither is a substitute for an Island lighting or
  gameplay-distance review.

## State and limitations

The asset is not placed in the Island map. The preview reads as deliberately
stylized and somewhat orderly; review a small, irregular woodland composition
before considering scatter. Its imported `.uasset` resides in the local
project's ignored `Content/` tree and is not part of this source commit. The
editable asset source is `Scripts/AssetPipeline/assets/woodland_fungus_cluster.py`;
rebuild it with:

```powershell
& 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe' -b --factory-startup `
  -P Scripts/AssetPipeline/run_asset.py -- woodland_fungus_cluster
& 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe' -b --factory-startup `
  -P Scripts/AssetPipeline/verify_asset.py -- WoodlandFungusCluster
```

The run modifies only the generated local export under `Saved/CaptiveSky/ComfyBlender/`;
it does not edit or save the Island map.
