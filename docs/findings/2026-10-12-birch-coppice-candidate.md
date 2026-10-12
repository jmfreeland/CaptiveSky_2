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

## Promotion gate

This is only an offline candidate: it has not been imported into Unreal,
reviewed under Island lighting, or placed in any map. Its materials are
parameter-only and refer to no shared texture files, so an isolated import
should not reimport the shared wood/canvas texture maps. Use a fresh destination
such as `/Game/Generated/ComfyBlender/BirchCoppice_Review`, then inspect close
and mid-distance silhouette, bark brightness, foliage shading, bounds, and
wind behavior. A sparse group belongs in a disposable Island copy first, with
the settled 30-FPS screen and existing viewpoint as comparison; no main-Island
placement should follow from the Blender preview alone.
