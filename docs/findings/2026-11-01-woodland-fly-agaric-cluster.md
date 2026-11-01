# Fly-agaric-inspired understory candidate — 2026-11-01

The existing `WoodlandFungusCluster` is a small, ochre-capped groundcover group.
This separate local candidate adds a more immediately recognizable red-cap
silhouette without changing the existing prop or Island map. It is a stylized,
fly-agaric-inspired decorative design, not a biologically complete model or a
foraging feature.

`Scripts/AssetPipeline/assets/woodland_fly_agaric_cluster.py` exports
`SM_WoodlandFlyAgaricCluster_01.fbx` to the ignored local output directory
`Saved/CaptiveSky/ComfyBlender/WoodlandFlyAgaricCluster/`. The five caps use
two related russet/scarlet tones and deterministic, gently varied ivory veil
fragments. The cluster is 31 × 22 × 19 cm, 2,220 triangles, four plain material
slots, `UVMap`, and no collision. All source meshes are triangulated before
FBX export so tangent-space export is clean.

Blender 5.2.2 re-imported the exported FBX, rendered its preview with Cycles on
CPU, and reported `PROBLEMS []`. The simple preview confirms the color/silhouette
contrast and exported material slots; it does not establish appearance in the
Island's lighting, under existing grass, or at normal visitor distance. The
small pale flecks and regular cap domes remain deliberately stylized. No Unreal
import, map placement, or runtime behavior has been validated. The live D-drive
project remains untouched; once an isolated Unreal review is safe, compare this
beside the ochre cluster in leaf litter and check scale, material response, and
grass occlusion before considering any placement.

Source-level tests: `Scripts/AssetPipeline/tests/test_woodland_fly_agaric_cluster.py`.
The full asset-pipeline suite passed (118 tests) under Blender 5.2's bundled
Python 3.11 runtime. The FBX and `Preview.png` are local ignored outputs; rebuild
from the tracked source with Blender 5.2 using `run_asset.py` and `verify_asset.py`.

## Isolated Unreal comparison workflow

`Scripts/Preview-WoodlandFlyAgaricReview.py` now provides three explicit Unreal
Python stages: `import` reuses the manifest-aware importer into the unique
`/Game/Generated/ComfyBlender/WoodlandFlyAgaricCluster` folder; `duplicate`
creates a non-overwriting copy of `/Game/Maps/Island`; and `place` adds the
collisionless red-cap candidate 160 cm beside the existing
`WoodlandFungusCluster_Review_01` in that copy. It refuses to overwrite an
existing review map or duplicate the comparison actor, and never saves the
source Island. The destination review map is
`/Game/Generated/ComfyBlender/WoodlandFlyAgaricReview_20261101/Maps/Island_WoodlandFungusAB_20261101`.

This workflow is prepared but **has not been run in Unreal**. The script passes
Python syntax compilation, and the two focused fly-agaric asset tests pass;
neither result validates Unreal import, material rendering, or the A/B scene.
After reviewing each stage in the editor, compare the existing ochre cluster
and red-cap candidate at the same visitor-scale angle before considering any
change to the main map.
