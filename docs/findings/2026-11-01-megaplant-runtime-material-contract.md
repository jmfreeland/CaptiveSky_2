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

## Direction

Do not replace this graph with a bare two-texture material: it would discard
the authored per-species seasonal, health, and translucency behavior. Next,
build a disposable project-local copy of the complete dependency closure and
remap the copied material/function references without changing the plugin
sample assets. Re-run this audit to prove the runtime closure is clear, compile
the copied material, then preview one instance-skinned tree before reparenting
the remaining species. Preserve the current parent graph, maps, and all
instance overrides until that preview has passed. No material assets were
created or changed in this milestone.
