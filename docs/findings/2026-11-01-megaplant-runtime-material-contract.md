# Megaplant runtime material contract (2026-11-01)

## Read-only audit

Expanded `Scripts/Inspect-MegaplantRuntimeMaterialDependencies.py` to include
the installed Common Hazel A tree and record each material instance's texture,
scalar, and vector overrides. Ran the script in the open UE 5.8.3 editor with
the Procedural Vegetation Editor sample assets mounted. The audit inspected
nine tree meshes and 18 slots, with no script errors; it did not save assets or
change the Island.

The current closure contains 24 PVE sample packages: the shared tree master,
three virtual textures, 19 material functions, and
`MPC_GlobalFoliageActor`. Twelve slots parent directly to the editor-only PVE
master. The other six use the project-owned
`MA_Foliage_Trees_InstancedSkinned` copy, but that parent still references the
same PVE closure. Those six copied-parent slots are the A/B bark slots for
Norway Maple, European Aspen, and European Beech.

Each audited bark/foliage material instance overrides species-specific base
color and normal textures. They also carry authored scalar/vector variation:
roughness, specular, health, translucency, season colors, and related controls.
Common Hazel follows the same texture contract. Its new material instance
references project-owned textures, so the shared parent graph—not missing
Hazel images—is the plugin dependency.

## Material-function graph feasibility

Extended the same read-only audit to traverse `MaterialExpressionMaterialFunctionCall`
nodes recursively using UE 5.8's `MaterialEditingLibrary` APIs. The editor log
for the nine-tree run reports 32 material/function graph assets and 62 function
calls; all calls resolved, and the Python wrapper exposed a callable function
setter on every call node (`unresolved_calls=0`, `missing_setters=0`). This is
evidence that nested function references can be inspected and likely remapped
in a project-local graph copy. No expressions were changed, and this does not
yet prove that copied graphs compile or render correctly.

The same run inventories 12 direct graph references: six to built-in Engine
textures and six references across four unique PVE assets—the three virtual
textures (`DefaultDiffuse_VT`, `Leaf_Mask_VT`, `Mask_VT`) and
`MPC_GlobalFoliageActor`. The references were found in the trunk, leaf, and
health/season functions. This matches the PVE-specific direct-reference subset
of the 24-package registry closure; function calls account for the remaining
PVE packages. The material-expression scan currently covers texture-base,
material-parameter-collection, and texture-collection properties, not every
possible asset-bearing expression class, so the registry closure remains the
broader dependency check.

Epic documents the collection expression's `collection` property in its
[`MaterialExpressionCollectionParameter` Python API](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/MaterialExpressionCollectionParameter);
the texture-base and texture-collection expression types are also present in
the [UE 5.8 Python API](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/index).

## Collision-checked runtime copy plan

The editor audit now checks every source package through the UE Asset Registry
and emits its actual class path, source object path, proposed destination, and
destination collision status. It verified 24 source packages / 24 objects:
three `Texture2D` virtual textures, one `Material`, 19 `MaterialFunction`
assets, and one `MaterialParameterCollection`. Every package contains exactly
one asset; every proposed destination package is empty; there are no exact
object-name collisions (`ready=24`, `destination_collision=0`,
`source_asset_missing=0`).

The reversible naming rule preserves the source-relative layout beneath
`/Game/Megaplant_Library/RuntimeMaterials/PVE/`. For example,
`/ProceduralVegetationEditor/SampleAssets/Materials/MasterMaterials/MA_Foliage_Trees.MA_Foliage_Trees`
maps to
`/Game/Megaplant_Library/RuntimeMaterials/PVE/SampleAssets/Materials/MasterMaterials/MA_Foliage_Trees.MA_Foliage_Trees`;
each of the three virtual textures and 19 nested functions follows the same
rule with its object name preserved. The plan is generated in
`runtime_copy_plan` by `Scripts/Inspect-MegaplantRuntimeMaterialDependencies.py`;
it performs no folder creation, asset copy, or save.

A filesystem preflight against the installed UE 5.8.3 plugin confirmed that all
24 source packages have their corresponding `.uasset` files under
`Engine/Plugins/Experimental/ProceduralVegetationEditor/Content`; the package
files total 2.41 MiB, with no `.uexp` or `.ubulk` companions. The plugin
descriptor separates `ProceduralVegetationEditor` (Editor module) from
`ProceduralVegetation` (Runtime module). Therefore the next test must verify
the copied graph in a runtime target without the editor module and determine
whether the runtime module can also be omitted; do not assume that removing the
entire plugin is valid from the content-package closure alone.

## Disposable test harness

Added `Playtests/MegaplantRuntimeMaterialSandbox/` with a minimal UE 5.8.3
project, `Scripts/Build-MegaplantRuntimeMaterialSandbox.py` to copy and remap
the 24 verified packages, and `Scripts/Verify-MegaplantRuntimeMaterialSandbox.py`
to load and recompile them with PVE disabled. Both scripts pass Python 3.11
syntax validation; the project descriptor parses and lists 24 unique source
packages. The first isolated `UnrealEditor-Cmd.exe -run=pythonscript -NullRHI`
attempt crashed during engine startup with an access violation at address zero,
before the script ran. No sandbox `Content/` directory or copied assets were
created. This is not evidence for or against material portability, and the
NullRHI startup path should not be retried without addressing the bootstrap
failure first.

Crash-report follow-up: the saved report records `SecondsSinceStart=0`, a read
access violation at null, and `PCallStackHash=F405610C114D120D4B6F5ABA901EC469AC9FAA26`.
The partial stack is in `UnrealEditor-Core` and `UnrealEditor-Cmd`; the crash
context's structured `CommandLine` field is empty even though the preceding
startup log records the full sandbox command. Engine symbols are not installed
with this UE distribution, so the offsets cannot currently be mapped to a
function. This narrows the failure to commandlet bootstrap, but does not explain
its cause.

The isolated UE 5.8.3 Editor target subsequently compiled and linked
successfully (44 UBT actions) against `Saved/CS58/CaptiveSky_2.uproject`.
This validates the current worktree C++ modules, including ecology and
resident tests, but does not execute Unreal automation or materialize the
sandbox assets. The open D: editor and its binaries were not changed.

## Status as of 2026-11-01

This cleared the package-mapping/collision gate, not the runtime-readiness gate.
Next, use the isolated project when a stable UE launch path is available to
materialize the closure, remap its graph references, then compile and preview
it with the editor module unavailable before touching live species parents. In
particular, verify virtual-texture support, the foliage collection's runtime
behavior, and whether the runtime module remains necessary. The original PVE
graph and Island remain unchanged.

## Initial direction (updated 2026-11-02 below)

Do not replace this graph with a bare two-texture material: it would discard
the authored per-species seasonal, health, and translucency behavior. Next,
build a disposable project-local copy of the complete dependency closure and
remap the copied material/function references without changing the plugin
sample assets. The disposable harness is prepared, but its first NullRHI launch
crashed before script execution; resolve that launch path or wait for a safe
rendered-editor window before running it. Re-run this audit to prove the
runtime closure is clear, compile the copied material, then preview one
instance-skinned tree before reparenting the remaining species. Keep the
editor-only module out of the runtime test and separately establish whether
the plugin's runtime module can be removed. Preserve the current parent graph,
maps, and all instance overrides until that preview has passed. No live Island
or source material assets were changed at that time. The graph API discovery
reduces the main implementation uncertainty but does not change this safety
gate.

## Material graph portability proof — 2026-11-02

Using `UnrealEditor-Cmd.exe -ExecutePythonScript` (not the failed
`-run=pythonscript` commandlet path), the disposable builder copied all 24
audited PVE packages, rewired 39 graph references, compiled the copied master
material, and reported `COPY_COMPLETE ... plugin_dependencies=0`. The first
correct editor invocation stopped at a DDC configuration fatal because the
project-local cache path exceeded UE 5.8's 119-character limit. Retrying with a
short temp DDC path and the normal Zen data directory completed successfully.

Added a sibling `_NoPVE.uproject` descriptor that leaves editor Python tools
available while disabling the ProceduralVegetationEditor plugin. The verifier
then loaded and recompiled the copied closure under that descriptor and
reported `NO_PLUGIN_VERIFY_COMPLETE assets=24 materials=1
plugin_dependencies=0`. The four focused Python harness tests also pass. This
proves that the copied shared master/function/texture/collection graph can be
loaded and recompiled without the PVE plugin; it does not yet prove rendered
appearance, material-instance override parity, or that the full tree assets
work in a packaged runtime. No live Island or source Megaplant asset was
modified.

The sandbox run generated a local `Config/DefaultEngine.ini` with a transient
Android File Server security token, so generated Config is ignored alongside
binary/cache outputs and is deliberately not committed.

Next: copy one A/B tree mesh and its material instance into the disposable
project, remap its foliage and bark slots to the copied parent while preserving
the audited per-species overrides, then verify dependencies with PVE disabled.
Keep actual rendering gated by at least 4 GiB of free GPU memory. Only after a
preview confirms leaf response and species color/season variation should any
live Island material parent change.
