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

## Follow-up compile finding

In the live UE 5.8.3 material editor, the root material's
`Used with Instanced Static Meshes` checkbox was confirmed unchecked. A
reversible in-editor toggle caused the root and its dependent instances to
compile for PCD3D_SM6, but compilation failed with `StaticSwitchParameter`
`Missing A input` and `MakeMaterialAttributes` `Error on property Normal`.
Unreal reports that it will use Default Material after this compile failure.

A read-only Python graph audit of an isolated project copy found 186
expressions and four `MaterialExpressionMaterialFunctionCall` nodes whose
`material_function` reference is `None` (expression names ending `_4` through
`_7`). Their outputs feed the True branches of static switches ending `_37`
through `_40`. The root's first `MakeMaterialAttributes` node takes its Normal
input from switch `_38` and World Position Offset from switch `_37`, explaining
why the unresolved billboard branches break both the material's Normal compile
and the `Billboard material` switch. The Python query completed; the scratch
commandlet exited non-zero later because its Derived Data Cache had no writable
node. This was diagnostic only, not a clean build.

The missing usage flag is therefore not the only issue, and enabling it alone
cannot restore the authored foliage appearance. The material-function assets
need to be recovered from the source package/library or a known-good project
copy before wiring or bypassing those branches.

The toggle was returned to unchecked. The on-disk root `.uasset` remains
byte-identical to the pre-test SHA256
`2B1746766E5EC27371AFDC7E58B7996A314320161A5CF943792A0D9CBCE055AB`.
A matching safety copy is in the ignored
`Saved/AssetBackups/Codex_RhodoMaterialUsage_20261008/` directory. No material
package was saved and no Game re-capture was run. The material editor still
shows a dirty package after recompilation; the user's separate unsaved Island
level was not saved or changed.

## Safe next action

Recover the four referenced material functions from the original plant package,
Fab library, or a known-good sibling project and restore their asset references.
Avoid wiring the false branch into both switch inputs without confirming the
intended billboard behavior. Compile the repaired graph before enabling
`MATUSAGE_InstancedStaticMeshes`.
Then save only the root material and run the same bounded Tideglass Game
capture, checking both that the 13 fallback warnings are gone and that the
vegetation visibly uses its authored material. Keep Starter Content `M_Rock`
and `M_Wood_Oak` checks separate.

## Scope and evidence

The initial audit used an isolated UE 5.8.3 Python commandlet and read the
shared asset dependency chain. The follow-up compile test used the existing
live editor. No binary `Content` asset or user project configuration was
modified on disk.

Game warnings are present in
[`Codex_TideglassCompositionCurrent_20261008.log`](../../Saved/Logs/Codex_TideglassCompositionCurrent_20261008.log)
and [`Codex_TideglassGameProfileCurrent_20261007.log`](../../Saved/Logs/Codex_TideglassGameProfileCurrent_20261007.log).
