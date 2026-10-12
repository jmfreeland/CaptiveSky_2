# Woodland hazel coppice candidate — 2026-10-12

## Purpose

After the earlier `WoodlandHazelThicket` failed its real-RHI Game review with a
sparse, skeletal crown, this separate collisionless candidate tested whether a
denser broadleaf silhouette could improve the woodland transition without
raising the global foliage count. The source is
`Scripts/AssetPipeline/assets/woodland_hazel_coppice.py`; no Unreal asset or
Island map was changed.

## Structural verification

Blender 5.2.2 built and re-imported the FBX successfully. The result measures
169 x 192 x 258 cm, has 12,310 triangles, one UV map, four parameter-only
material slots, `ZMIN=0.0637 m`, and no collision meshes. The verifier reported
`PROBLEMS []` / `VERIFY DONE`. Build output and preview are local under
`Saved/CaptiveSky/ComfyBlender/WoodlandHazelCoppice/` (gitignored).

## Visual decision

The re-imported Blender preview still shows conspicuous branch ends and
isolated, angular leaf sprays instead of one continuous, naturally layered
hazel crown. It is rejected for Island placement. This was not imported into
Unreal, so material response, wind, Game appearance, HISM cost, and performance
remain untested; the Blender render is enough to stop this art direction before
spending another unstable Unreal session on it.

Preview: [re-imported FBX render](../../Saved/CaptiveSky/ComfyBlender/WoodlandHazelCoppice/Preview.png).

## Next step

Do not solve the mid-distance woodland gap by multiplying hand-built leaf
geometry or increasing global plant counts. Prefer a small proven in-game
foliage form combined into an art-directed group, or resume the isolated UE 5.8
PVE Static Mesh export when the editor is stably targetable. Any accepted
candidate still needs the same Tideglass-to-Woodland Game view, material/wind
review, HISM/PCG compatibility check, and a bounded performance comparison
before Island placement.
