# Woodland bluebell clump candidate

**State:** isolated asset candidate; imported for review, not placed in the Island.

Added `Scripts/AssetPipeline/assets/woodland_bluebells.py` as a deterministic,
script-built flowering ground-cover species. The clump has seven stems, low
strap leaves, drooping lavender bells, three parameter-only material slots,
and no collision. It is intended as a shaded woodland accent that can later
be distributed in restrained PCG/HISM patches rather than used as a large
landscape carpet.

## Validation (2026-10-12)

- Blender 5.2.2 exported the FBX and re-imported it for the preview. The
  measured bounds are 62.7 x 50.6 x 36.0 cm; origin is at ground level
  (`ZMIN 0.0054 m`), there is one `UVMap`, three material slots, 5,116
  triangles, and `PROBLEMS []`.
- Unreal 5.8.3 commandlet imported it to the fresh review folder
  `/Game/Generated/ComfyBlender/WoodlandBluebells_Review_20261012` with
  `validation_errors: []`, matching dimensions and all three material
  assignments, and zero collision shapes. It ran with `NullRHI`; no map was
  opened or saved.
- Evidence: local-only
  `Saved/CaptiveSky/ComfyBlender/WoodlandBluebells/Preview.png`,
  `unreal_import.json`, and
  `Saved/Logs/Codex_WoodlandBluebellsImport_20261012.log`.

### UE real-RHI visual gate

An isolated UE 5.8.3 render was captured beside `Roost_West` using transient
actors; the Island map was not saved. The ordinary final-color capture at
`Saved/Playtests/Codex_WoodlandBluebellRealRHI_20261012/StaticMesh_RealRHI.png`
shows the mesh, but its foliage and flowers are nearly monochrome with severe
dark fragments and hard shadows. A second `SCS_BASE_COLOR` capture at
`StaticMesh_BaseColor.png` is also neutral gray. The source manifest specifies
green leaf/stem and blue-violet bloom colors, so this does not pass visual
review and cannot be explained away as only the scene's lighting. The exact
cause (material compilation/binding versus capture behavior) remains
unresolved; inspect the imported mesh's material slots and material outputs
before changing geometry or placing it in PCG. Capture log:
`Saved/Logs/Codex_WoodlandBluebellBaseColor_20261012.log` (the earlier
final-color run is in `Codex_WoodlandBluebellRealRHI_ObjectSpawn_20261012.log`).

## Remaining review

The FBX preview is a deliberately stylized low-poly interpretation, not a
photoreal botanical asset. Its 5.1k triangles per clump are high for dense
ground cover; if approved for placement, test a lower-poly variant or keep
patch density sparse. Material/color appearance is currently a failed visual
gate; translucency/two-sided foliage settings, wind response, LODs, PCG
distribution, and gameplay frame-time are also unvalidated. Do not place it in
the Island until the neutral-gray capture is diagnosed and a naturally lit
viewpoint passes review.
