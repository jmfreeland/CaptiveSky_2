# Everestianum material fallback in instanced foliage

## Finding

Recent bounded Game logs repeatedly warn that 13 materials used by the imported
Everest rhododendron are missing the `InstancedStaticMeshes` usage and will
render with Unreal's Default Material. A read-only UE 5.8.3 asset-registry and
Python audit traced every warned material instance through
`MI_Pivot_Rhododendron__Everestianum__HD` to the single root material
`/Game/Plants/Materials/OriginalFlower/MM_Rhododendron__Everestianum__HD`.
The root material's `bUsedWithInstancedStaticMeshes` property is `False`.
Changing this one root should address the 13 inherited plant-material
warnings, rather than modifying the 13 instances individually.

The same Game logs separately report Starter Content `M_Rock` and
`M_Wood_Oak` missing the usage flag. They are separate assets and should be
audited/repaired independently if those instanced materials remain in use.

## Safe next action

Do not write the `.uasset` files from a second editor/commandlet while the live
editor is open and reports unsaved state. In the active editor, use Unreal's
material usage API/editor control to enable
`MATUSAGE_InstancedStaticMeshes` on the rhododendron root material, save that
material only, and allow shader recompilation. Then run the same bounded
Tideglass Game capture and verify both that the 13 fallback warnings are gone
and that the vegetation renders with its authored material. Keep the
`M_Rock` and `M_Wood_Oak` checks separate so their appearance/performance impact
is not conflated with the plant fix.

## Scope and evidence

The investigation was read-only: no binary `Content` asset or user project
configuration was modified. The audit used an isolated UE 5.8.3 Python
commandlet and read the shared asset dependency chain; the material usage flag
was false. The current editor showed one unsaved item, so the asset was not
saved from the audit process.

Game warnings are present in
[`Codex_TideglassCompositionCurrent_20261008.log`](../../Saved/Logs/Codex_TideglassCompositionCurrent_20261008.log)
and [`Codex_TideglassGameProfileCurrent_20261007.log`](../../Saved/Logs/Codex_TideglassGameProfileCurrent_20261007.log).
