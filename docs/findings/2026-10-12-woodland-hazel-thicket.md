# Woodland hazel thicket asset candidate (2026-10-12)

The Tideglass-to-Woodland Game frame shows a broad open midground below a thin,
repeating tree line. To explore a local, lightweight way to break up that
transition, `Scripts/AssetPipeline/assets/woodland_hazel_thicket.py` now builds
a collisionless, multi-stem deciduous shrub mass. It is intended for sparse
woodland-edge groups and potential small-animal cover, not a global increase in
foliage density.

## Asset verification

Blender 5.2.2 exported and then re-imported the FBX for its preview. The
measured dimensions are 193.4 x 186.8 x 228.8 cm, with the origin grounded at
`ZMIN=0.0213 m`. The mesh has one `UVMap`, four parameter-only material slots,
4,914 triangles, and no collision meshes. The verifier reports
`PROBLEMS []` / `VERIFY DONE`. The local export and preview are under
`Saved/CaptiveSky/ComfyBlender/WoodlandHazelThicket/` (gitignored).

## Review limits and next gate

This is only a stylized source-mesh candidate. Its preview still shows distinct
bare stems and angular leaves; it may read as a loose low-poly thicket rather
than natural hazel at close range. The UE bridge is unavailable, so it has not
been imported, viewed with the Island's actual lighting/material pipeline,
tested for HISM draw cost, or compared against existing woodland vegetation.
It has not been placed on the Island. Do not treat Blender's FBX check as proof
of acceptable game art. Next, import it to a fresh review folder and compare a
single mesh and a tiny sparse HISM group at the same woodland-transition view;
reject or revise it if it looks too stylized or disrupts the existing cover.
