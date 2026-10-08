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
`material_function` reference is `None` and zero saved input names (expression
names ending `_4` through `_7`). Their outputs feed the True branches of static
switches ending `_37` through `_40`. The root's first `MakeMaterialAttributes`
node takes its Normal input from switch `_38` and World Position Offset from
switch `_37`; the material editor identifies the affected switch as
`Billboard material`.

The package's Engine dependencies include several similarly named
`ExampleContent` assets, but loading those exact packages in UE 5.8.3 resolves
them as `Texture2D`, not `MaterialFunction`. They are not valid replacements for
the empty call nodes. Searches across the local CaptiveSky/CS_Interactive
project copies found the same root material hash in each; no clean donor copy
was found. A filename search across the local Fab VaultCache, Documents, and
Downloads also found no separate Everestianum source package. The Python graph
query completed; the scratch commandlet exited non-zero later because its
Derived Data Cache had no writable node. This was diagnostic only, not a clean
build.

The missing usage flag is therefore not the only issue, and enabling it alone
cannot restore the authored foliage appearance. The four call nodes have no
function references or input pins to relink; a known-good source package is
needed to restore them accurately, or they must be deliberately reconstructed
on a test duplicate before any live replacement.

The toggle was returned to unchecked. The on-disk root `.uasset` remains
byte-identical to the pre-test SHA256
`2B1746766E5EC27371AFDC7E58B7996A314320161A5CF943792A0D9CBCE055AB`.
A matching safety copy is in the ignored
`Saved/AssetBackups/Codex_RhodoMaterialUsage_20261008/` directory. No material
package was saved back to `Content`, and no Game re-capture was run. Unreal did
create `Saved/Autosaves/Game/Plants/Materials/OriginalFlower/MM_Rhododendron__Everestianum__HD_Auto2.uasset`
during the test. The material editor still shows a dirty package after
recompilation; the user's separate unsaved Island level was not saved or
changed.

## Safe next action

Obtain the original plant source package or a genuinely different known-good
project copy. If none is available, reconstruct the four empty call nodes on a
test duplicate and validate their visual/compile behavior there; do not connect
the similarly named texture assets or blindly copy false branches. The existing
`Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/Project` is not an
isolated asset workspace: its `Content` directory is a junction to the live
project's `Content`. Do not save or otherwise mutate assets through that
project. A follow-up experiment needs a physical scratch `Content` directory
with a copied material package. Compile the repaired graph before enabling
`MATUSAGE_InstancedStaticMeshes`.
Then save only the root material and run the same bounded Tideglass Game
capture after coordinating any write to the shared `Content/` asset. Check both
that the 13 fallback warnings are gone and that the vegetation visibly uses its
authored material. Keep Starter Content `M_Rock` and `M_Wood_Oak` checks separate.

## Scratch-project isolation correction

On 2026-10-08, checking the scratch project's filesystem metadata showed that
`Project/Content` is a Windows junction targeting the main project's `Content`
directory. The earlier Python graph and registry inspections were read-only,
and the root package still matches its pre-test SHA256, so there is no evidence
those inspections wrote to the live asset. Nevertheless, that scratch project
must not be treated as safe for material-editing tests: writes through the
junction would affect shared content. The live material editor also remains
open with a dirty in-memory package from the compile test; do not save it or
save the user's separate unsaved Island level as part of this investigation.

## Scope and evidence

The initial audit used a UE 5.8.3 Python commandlet whose scratch project's
`Content/` junction shared the main asset tree; the audit only read the asset
registry and material graph. The follow-up compile test used the existing live
editor. No binary `Content` asset or user project configuration was modified
on disk.

Game warnings are present in
[`Codex_TideglassCompositionCurrent_20261008.log`](../../Saved/Logs/Codex_TideglassCompositionCurrent_20261008.log)
and [`Codex_TideglassGameProfileCurrent_20261007.log`](../../Saved/Logs/Codex_TideglassGameProfileCurrent_20261007.log).
