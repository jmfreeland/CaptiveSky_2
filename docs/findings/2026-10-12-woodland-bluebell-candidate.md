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
`StaticMesh_BaseColor.png` also appears neutral gray. A UE-side audit then
confirmed all three asset slots and the spawned component slots reference the
expected materials; each material's connected base-color node contains the
manifest value (stem `(0.20, 0.31, 0.11)`, leaf `(0.11, 0.23, 0.075)`, bloom
`(0.24, 0.20, 0.52)`). LOD0 has three render sections. This rules out missing
material assets and missing component assignments, but does not yet prove the
section-to-slot mapping. A full-scene SceneCapture attempt also produced a
heavily occluded, partly black frame
(`StaticMesh_FullSceneAudit.png`), so this preview method is not reliable
enough to judge the in-game colors. Do not retune source colors based on these
captures. The exact issue—section mapping or a SceneCapture/render-path
difference—remains unresolved; verify in a normal editor/game viewport before
placing the candidate in PCG. Audit log:
`Saved/Logs/Codex_WoodlandBluebellSectionAudit_20261012.log`; the base-color
material values are in `Codex_WoodlandBluebellMaterialInputAudit_20261012.log`.
The earlier captures are in `Codex_WoodlandBluebellBaseColor_20261012.log` and
`Codex_WoodlandBluebellRealRHI_ObjectSpawn_20261012.log`.

## Remaining review

The FBX preview is a deliberately stylized low-poly interpretation, not a
photoreal botanical asset. Its 5.1k triangles per clump are high for dense
ground cover; if approved for placement, test a lower-poly variant or keep
patch density sparse. In-game material/color appearance is unverified because
the transient SceneCapture diagnostics are unreliable; translucency/two-sided
foliage settings, wind response, LODs, PCG distribution, and gameplay
frame-time are also unvalidated. Do not place it in the Island until a normal
viewport or gameplay viewpoint passes review.

## Follow-up: bounded runtime integration (2026-10-17)

The later change `5d3f86a` did integrate this mesh as the ninth deterministic
meadow-flower variant. It replaces another species at an existing candidate
site; it does not add a separate scatter pass, increase the site budget, or
raise the per-site flower cap. The focused
`CaptiveSky2.Agent.MeadowFlowerSpecies` test passed against the isolated UE
5.8.3 scratch build under NullRHI, confirming that the imported mesh resolves
and its HISM is non-colliding and excluded from navigation. This resolves the
placement-history discrepancy, but not the visual gate described above:
NullRHI cannot judge material appearance, player-scale readability, or the
cost of the 5,116-triangle clump in a rendered scene. Treat bluebells as an
integrated but visually unapproved variant until a normal viewport/gameplay
review and a frame-time check are available. See also
[`meadow-flower-clusters.md`](../ecology/meadow-flower-clusters.md).
