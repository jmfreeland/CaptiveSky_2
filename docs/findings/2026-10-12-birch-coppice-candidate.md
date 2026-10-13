# Silver-birch coppice candidate (2026-10-12)

The Tideglass-to-Woodland Game capture shows an open brown midground beneath a
thin, repetitive tree line. As a small species-diversity experiment,
`Scripts/AssetPipeline/assets/birch_coppice.py` builds a five-stem,
silver-birch-inspired coppice with pale bark and light-green foliage. It uses
the project's no-credit Blender asset helpers and is intended as a sparse
woodland-edge group, not a global foliage-density increase.

## Offline export checks

Blender 5.2.2 exported the mesh to the local, gitignored pack
`Saved/CaptiveSky/ComfyBlender/BirchCoppice/`. The FBX re-imported in Blender
for verification with:

- bounds about 147 × 146 × 277 cm and `ZMIN=0.0003 m`;
- 27,640 triangles, one `UVMap`, and five parameter-only material slots;
- no collision meshes and `PROBLEMS []`.

The re-imported [Blender preview](../../Saved/CaptiveSky/ComfyBlender/BirchCoppice/Preview.png)
shows an airy, stylized crown with pale stems. The uneven branch placement
breaks the earlier tiered silhouette, but the result still reads as a
low-poly game tree rather than finished botanical art.

## Unreal review

UE 5.8.3 imported the candidate to the fresh local-only destination
`/Game/Generated/ComfyBlender/BirchCoppice_Review/SM_BirchCoppice_01`.
The import report records 146.7 × 145.8 × 276.8 cm, 27,640 source triangles,
five assigned constant-color materials, no imported textures, no shared-library
texture references, and no collision. The Unreal log confirms the expected base
colors and material assignments. This avoided reimporting shared wood/canvas
texture maps.

Two transient, unsaved scene-capture attempts were made. The first real-RHI
capture placed the clump beside `Roost_West`, but the tree and nearby structure
fell into such deep shadow that the result is not a fair material review:
[Roost_West preview](../../Saved/Playtests/Codex_BirchCoppiceRealRHI_20261012/001_RoostWest.png).
A second capture attempted an explicit noon-light override; it rendered almost
entirely black (sampled maximum channel 11/255) and is invalid as appearance
evidence: [noon preview](../../Saved/Playtests/Codex_BirchCoppiceNoon_20261012/001_ListeningStones.png).
The Island map was never saved or modified by either preview.

## Promotion gate

Keep this as a review-only asset for now. Import validation passed, but visual
quality in the actual Island lighting, mid-distance readability, wind response,
and instanced-material compatibility remain unverified. The dark Roost capture
and failed noon capture mean we cannot yet tell whether the asset or the capture
lighting is responsible. Before placement, fix the daylight capture path and
review the silhouette in an open woodland-edge setting; then test a sparse group
in a disposable Island copy against the established viewpoint and 30-FPS
screen. Do not add it to the authored Island from the Blender preview alone.
