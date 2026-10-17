# Tideglass marram-grass candidate — 2026-10-17

Added `Scripts/AssetPipeline/assets/tideglass_marram_grass.py` as a local,
deterministic marram-grass clump for exposed, salt-sprayed shoreline ground.
The cool-green arching blades and wind-combed seed spikes give a different
silhouette and seasonal cue from the low pink sea-thrift tuft. Geometry uses
three flat material slots and no generated textures; it has no collision and
does not affect navigation, resident state, or model-request count.

The focused source contract passed all three tests. Blender 5.2.2 exported
`Saved/CaptiveSky/ComfyBlender/TideglassMarramGrass/SM_TideglassMarramGrass_01.fbx`
at 108 × 97 × 74 cm, 3,078 triangles, and three material slots. Re-importing
the FBX through the local verifier retained `UVMap`, reported no collision,
and finished with `PROBLEMS []`; `ZMIN` is 0.012 cm. The
[`Preview.png`](../../Saved/CaptiveSky/ComfyBlender/TideglassMarramGrass/Preview.png)
shows a compact nine-blade clump with three pale seed spikes. This is a
Blender render, not evidence of how the flat palette reads under Unreal
lighting or among the existing Tideglass vegetation.

The candidate remains outside `Content/` and is not placed on the Island.
After the editor recovers, import to a fresh generated-asset destination and
review the material, scale, and silhouette in the actual shoreline context.
Only then consider a sparse nonblocking HISM strip behind the sea-thrift
flowers; do not save a map placement from this Blender preview alone.
