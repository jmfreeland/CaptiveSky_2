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

Next, verify the full 24-package source-to-destination mapping, including the
master material and every function, before copying/remapping into a disposable
project folder. Compile and preview there before touching the live species
parents. The original PVE graph and Island remain unchanged.

## Direction

Do not replace this graph with a bare two-texture material: it would discard
the authored per-species seasonal, health, and translucency behavior. Next,
build a disposable project-local copy of the complete dependency closure and
remap the copied material/function references without changing the plugin
sample assets. Re-run this audit to prove the runtime closure is clear, compile
the copied material, then preview one instance-skinned tree before reparenting
the remaining species. Preserve the current parent graph, maps, and all
instance overrides until that preview has passed. No material assets were
created or changed in this milestone. The graph API discovery reduces the
main implementation uncertainty but does not change this safety gate.
