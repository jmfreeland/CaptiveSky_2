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

## Source-pattern reconstruction on a disposable duplicate (2026-10-12)

A new read-only UE 5.8.3 comparison of the healthy Festuca, Phalaris, and Typha
master graphs found the same resolved PlantFactory calls at slots 4–7 in all
three. The Rhododendron graph retains the exact corresponding upstream nodes,
and its four billboard static switches still select the matching null call in
their True branch. Reconstructing from these peer graphs is therefore
source-supported, unlike wiring the unrelated similarly named textures.

On the isolated duplicate
`/Game/Generated/MaterialTests/Codex_RhodoMaterialRepair_20261012/MM_Rhodo_InstancedRepair_Test`
only, the calls were restored as:

- Calls 4 and 6: `StaticBillboard`, with `TextureCoordinate_2/3` connected to
  `position uv` and `ComponentMask_0/1` connected to `hook`.
- Calls 5 and 7: `BillboardMappedNormals`, with `StaticSwitchParameter_4/20`
  connected to `normal`.

The duplicate's four input mappings were read back and matched all four expected
sources; its `InstancedStaticMeshes` usage flag is enabled. UE's
`recompile_material` returned an empty diagnostic list under the PCD3D_SM6
shader platform. Evidence: `Saved/Logs/Codex_RhodoPeerPins_20261012_final.log`,
`Saved/Logs/Codex_RhodoDuplicateRepair_Retry_20261012.log`, and
`Saved/Logs/Codex_RhodoDuplicateCompile_20261012.log`. The original material
still has SHA256 `2B1746766E5EC27371AFDC7E58B7996A314320161A5CF943792A0D9CBCE055AB`;
it was not edited. This validates graph reconstruction and a clean SM6
recompile on the duplicate, **not** rendered appearance or the removal of live
Game fallback warnings. The compile probe did not load the Island; its 4 GiB
pool setting is not a texture-budget measurement. Next, visually preview the
duplicate's material instances on the Rhododendron mesh in an isolated test
view, then coordinate before applying the verified graph to the shared source
material.

## Isolated preview attempt stopped for outbound LLM requests (2026-10-12)

An isolated preview level and duplicate material instances were created under
`/Game/Generated/MaterialTests/Codex_RhodoMaterialRepair_20261012/`; neither
the Island map nor the original Rhododendron material was changed. While the
preview commandlet was running, its log recorded 13 POST attempts to
`https://api.openai.com/v1/chat/completions` (followed by retries). Each logged
attempt failed at connection time with `Bad access` / `Could not connect to
server`; there is no successful HTTP response in the log. The process was
stopped, and no preview image was produced. Evidence:
`Saved/Logs/Codex_RhodoPreviewBuild_Retry_20261012.log`.

The cause is not established; the endpoint and timing are insufficient to
attribute the requests confidently to resident thinking, Semantic Search, or
the MCP plugin. The project default allows up to 120 resident requests per
play, so a future diagnostic run must explicitly set
`-CaptiveSkyMaxModelRequests=0` and verify that the relevant provider cannot
send asset or world context before attempting another capture. Do not resume the
render test until that is confirmed or the user explicitly approves the
specific outbound model use. The preview commandlet reported a 4 GiB streaming
pool, but it did not load the Island and did not measure the Island's texture
budget.

Follow-up code inspection found that the brain and sleep-consolidation paths
treated a missing play-session subsystem as permission to continue dispatching.
Both now use a shared fail-closed reservation gate; the safety test covers a
missing subsystem and a zero-request cap. The editor-target build succeeded and
`CaptiveSky2.Agent.SessionSafety` passed in a NullRHI run with a 60-second cap
and zero permitted model requests. The test log contains no OpenAI chat-completion
endpoint attempts. This closes the identified missing-guard path, but does not
identify which subsystem initiated the earlier preview attempts.

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
