# Woodland floor diversity: wild-strawberry candidate

## Decision

Add a reusable low woodland-floor plant alongside the existing fern and
bluebell clumps. The candidate is a small wild-strawberry patch: three loose
trifoliate rosettes, three flowers, and two low-set ripe fruits. Its intended
role is a scattered woodland-edge/clearing accent, not a continuous carpet.
The groundcover has no collision so it cannot obstruct walking characters.

Source of truth: `Scripts/AssetPipeline/assets/woodland_wildstrawberry.py`.
The asset is deterministic and uses the existing local ComfyBlender toolkit;
there is no external model-generation call or paid asset dependency. Its
source-level contract is in
`Scripts/AssetPipeline/tests/test_woodland_wildstrawberry.py`.

## Verification and limits

All three source contract tests pass. They check the fixed species
composition, the five material roles, and the absence of collision-generation
calls. Blender 5.2.2 LTS built and exported the asset in an isolated
`Saved/CaptiveSky/ComfyBlender/Codex_WoodlandWildStrawberry_20261013/` root.
The full asset-pipeline `unittest` suite passed all 27 tests when its temporary
fixtures were redirected to a project-local ignored directory (the Windows
sandbox's default temp path denied fixture writes). The FBX re-import/render
verification passed with no reported problems:

- Dimensions: 0.78 × 0.629 × 0.217 m; ZMIN 0.0146 m.
- 2,038 triangles; five expected material slots; UVMap present.
- No collision meshes; `PROBLEMS []`.
- [Re-imported Blender preview](../../Saved/CaptiveSky/ComfyBlender/Codex_WoodlandWildStrawberry_20261013/WoodlandWildStrawberry/Preview.png).

In this standalone preview it reads as a low, asymmetrical flowering
strawberry patch with visible red fruit. This is only a source-asset silhouette
check: there is no Island ground/context, no Unreal material/import check, and
no player-scale legibility evidence. No `Content/`, Island map, resident
memory, or plant placement was changed. The export remains a local ignored
artifact; only its deterministic source and test are tracked candidates.

## Next step

After the existing UE 5.8.3 project-startup crash is diagnosed, import the FBX
as a disposable candidate and compare it in the fixed Tideglass-to-Woodland
Game view at close and middle distance. Judge whether its leaves, flowers and
fruit remain legible beside the already-proven habitat assets. Do not scatter
it or treat it as Island biodiversity until that Unreal review passes.
