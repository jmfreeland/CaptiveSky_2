# Tideglass marsh marigold: zero-width topology

The leaf and sepal width profiles intentionally taper to zero at both ends.
The former builder still emitted a full row of coincident vertices at those
endpoints, so triangulation produced collapsed faces and Unreal reported
near-zero normal/tangent data during import. Unreal's earlier import also
reported fewer triangles than the source manifest, consistent with the
degenerate endpoint faces being discarded.

The builder now creates one apex vertex at each zero-width endpoint and joins
the adjacent row with a triangle fan. Intermediate rows retain their existing
quad topology. FBX export now carries face-smoothing information as well.
This leaves the flower and leaf silhouette unchanged while making the endpoint
topology explicit.

## Verification

- Blender 5.2.2 headless export: 4,688 triangles, four material slots, 76 x 78
  x 39 cm, no collision meshes.
- `Scripts/AssetPipeline/verify_asset.py`: `PROBLEMS []`; FBX re-imported with
  one UV layer and four expected materials; preview rendered successfully.
- Independent FBX re-import inspection: 0 zero-area polygons, 0 zero-length
  normal loops, and 0 zero-length tangent loops.
- Asset pipeline Python suite: 86 tests passed.
- The rendered preview retains the previous silhouette and proportions.

## Limits and next step

This is Blender/FBX evidence only. The current desktop bridge exposes no Unreal
window, and no Unreal process is visible in the current session, so this build
has not yet been imported into Unreal. Keep existing review assets and the
Island map untouched. When the editor is accessible, import into a new,
separate review folder and confirm the FBX warnings are gone before deciding
whether to replace the earlier review asset or place anything in the map.
