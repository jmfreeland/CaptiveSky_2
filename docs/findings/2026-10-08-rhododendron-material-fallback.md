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
This means the missing usage flag is not the only issue, and enabling it alone
cannot restore the authored foliage appearance.

The toggle was returned to unchecked. The on-disk root `.uasset` remains
byte-identical to the pre-test SHA256
`2B1746766E5EC27371AFDC7E58B7996A314320161A5CF943792A0D9CBCE055AB`.
A matching safety copy is in the ignored
`Saved/AssetBackups/Codex_RhodoMaterialUsage_20261008/` directory. No material
package was saved and no Game re-capture was run. The material editor still
shows a dirty package after recompilation; the user's separate unsaved Island
level was not saved or changed.

## Safe next action

Inspect the root graph and the referenced static switch/material-attributes
nodes; identify the intended source for the missing `A` input and `Normal`
attribute from the imported material graph or a known-good sibling material.
Repair and compile the graph before enabling `MATUSAGE_InstancedStaticMeshes`.
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
