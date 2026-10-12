# Procedural Vegetation Editor availability (2026-10-10)

UE 5.8.3 includes the Experimental `ProceduralVegetationEditor` plugin at `Engine/Plugins/Experimental/ProceduralVegetationEditor`. Its descriptor is marked `IsExperimentalVersion: true` and `EnabledByDefault: false`. `CaptiveSky_2.uproject` opts in, and a later read of the live editor confirms it is enabled there now.

**Post-restart live check (2026-10-11):** Unreal MCP reported `/Game/Maps/Island`, PIE stopped, and `ProceduralVegetationEditor` enabled; its metadata resolves to the UE 5.8 engine plugin. The active session does **not** discover or mount `PlantFactoryPlugin` (`IsEnabled` and `/PlantFactoryPlugin/` lookup both fail), although the main `.uproject` still declares it enabled and the local content-only plugin descriptor exists under `Plugins/PlantFactoryPlugin/`. The active process was observed starting at 10:59, but its project identity could not be read independently, so do not infer that this confirms the main project's PlantFactory mount. Bentley states PlantFactory development is discontinued and lists its Unreal integration as compatible only through UE 5.3; treat it as legacy, not a UE 5.8 dependency ([Bentley product FAQ](https://www.bentley.com/products/e-on-software/)). Important distinction: the tracked `PlantFactoryPlugin` here is only a content mount, not the PlantFactory application or executable integration. The existing plant-material audit found original material-function references in Festuca, Phalaris, Typha, and Rhododendron graphs; restoring the mount resolved the PlantFactory functions for the first three, while four Rhododendron calls remain null (see [2026-10-04 audit](2026-10-04-plant-material-dependencies.md)). Removing the mount alone would reintroduce missing material-function references. Preserve it until those plant materials are replaced or deliberately removed; PVE itself is independently available.

**Local species inventory (read-only, 2026-10-11):** the live asset registry exposes four Megaplant PVE graph families—Common Hazel, European Aspen, European Beech, and Norway Maple—and each has A/B/C/D output assets currently classified as `SkeletalMesh`. The Hazel graph's dependencies are `/Game/Megaplant_Library` assets and UE's PVE/PCG sample assets; no `/PlantFactoryPlugin/` dependency appeared in that graph's dependency list. This is a promising diversity source, but not yet a runtime-ready HISM set: export one deliberately small standard Static Mesh specimen from a clean graph copy, then validate silhouette, material resolution, LOD/Nanite, wind, bounds, and a tiny isolated scatter before adding any species to the Island. No Island placement or source-graph edit was made during this inventory.

**Earlier scratch-session reconciliation (2026-10-11, superseded by the
post-restart check above):** an earlier Unreal MCP session was running
`Saved/CompileScratch/Claude_Props/CaptiveSky_2.uproject`, confirmed by that
session's `LogCsvProfiler` command line. Its descriptor omitted both plugins
and PVE was disabled there. That observation was specific to the scratch
session; the tracked main `CaptiveSky_2.uproject` declares both plugins enabled.
Do not use the scratch-session result as evidence about the editor after the
10:59 restart. No plugin toggle or editor restart was performed in that earlier
audit.

To check compatibility first, I enabled the plugin in the ignored isolated scratch descriptor at `Saved/CompileScratch/RatAnimationProbe_20261010/CaptiveSky_2.uproject`. The installed engine mounted `ProceduralVegetationEditor` and loaded both `UnrealEditor-ProceduralVegetation.dll` and `UnrealEditor-ProceduralVegetationEditor.dll`. UBT accepted the PVE-enabled project configuration, and the scratch editor completed the existing `CaptiveSky2.Agent.RatLocomotionAnimation` test under `NullRHI`, disabled resident thinking, zero model requests, and a 60-second cap ([log](../../Saved/CompileScratch/RatAnimationProbe_20261010/PVEAvailability.log)). After that compatibility check, I enabled PVE in the main `.uproject`. The post-restart live MCP check reports PVE enabled, but did not independently identify the active project path. The local `PlantFactoryPlugin` content-only mount was successfully mounted in the isolated scratch probe, but is not discovered by the post-restart live session; the main descriptor still lists it as enabled. No Content or Island state was changed in the compatibility check.

This proves the installed UE 5.8.3 PVE plugin can load in an isolated scratch copy and is enabled in the post-restart live session; it does not prove interactive vegetation authoring, generated asset quality, Nanite suitability, compatibility with the existing Fab Megaplants, or runtime performance. The current editor also reports `/Game/Characters/MetaHumans/MHC_Aster` dirty, so leave that asset untouched. The earlier export-dialog attempt produced no mesh; the saved `PVE_Hazel_StaticMeshExport_Test` graph is still only a test copy. Next: once the Unreal MCP endpoint is live and the dirty Aster state is safe, inspect the export dialog on the test copy, explicitly choose standard Static Mesh, and produce a disposable preview. Do not edit the source graphs or place candidates on the Island until the exported mesh passes material, bounds, LOD/Nanite, wind, and small-scatter checks.

The first PVE probe lacked the project-local `Plugins/PlantFactoryPlugin` directory and therefore reported its material-function references missing. I corrected the scratch setup with a junction to the main project's content-only plugin (no assets copied or modified) and reran startup. `PVEPlantSupportProbe.log` now confirms both `ProceduralVegetationEditor` and `PlantFactoryPlugin` mount, the PVE editor module loads, the plant packages scan without `LoadErrors`, and `CaptiveSky2.Agent.RatLocomotionAnimation` passes. This validates plugin discovery and package-mount compatibility in the isolated scratch project; it does not validate interactive PVE authoring or rendered/runtime behavior in the live project.

**Static-mesh export route (2026-10-11):** Epic's [UE 5.8 PVE guide](https://dev.epicgames.com/documentation/unreal-engine/procedural-vegetation-editor-pve-in-unreal-engine) states that the PVE Export node can bake either Nanite Foliage or a standard static mesh, and that graph output itself should not be placed in the world. This gives the Island's existing HISM/PCG scatter a plausible integration route: duplicate one plant graph, export one standard static-mesh specimen, then validate its bounds, materials, LODs/Nanite behavior, and wind before testing a tiny disposable PCG/HISM preview. This is not yet demonstrated: the live editor still does not discover the content-only `PlantFactoryPlugin` mount, and `/Game/Generated/ProceduralVegetation/` contains the PVE graphs but no PVE-exported static-mesh output. Do this only after the currently open editor assets are saved/closed or the user explicitly wants that session used; do not edit the Island map as part of the export experiment.

**Interactive export dialog probe (2026-10-11; incomplete):** The live editor could open the clean project-local `/Game/Generated/ProceduralVegetation/PVE_Common_Hazel_01` graph, and I duplicated it to `/Game/Generated/ProceduralVegetation/PVE_Hazel_StaticMeshExport_Test` without changing the source. Its Export action opened `Modify Export Settings`; the current defaults were output folder `/Game/Generated/ProceduralVegetation`, mesh name `Tree_Common_Hazel_01_A`, replace policy `Replace`, and export type `Skeletal Mesh` (not yet changed to Static Mesh). The selected specimen reports 10,057 points, 381 branches, 201,072 trunk vertices, 389,150 trunk triangles, 1,855 bones, nine unique foliage meshes, and 923 foliage instances, so this is not a lightweight export candidate. A `dotnet.exe - Application Error` popup with exception `0xe0434352` appeared over the dialog and reappeared while it remained open; causation is unconfirmed. A read-only query of the Windows Application log found no `.NET Runtime` or `Application Error` events in the two-hour window around this probe, so there is no event-log evidence identifying the failing process or exception type. No export completed and no output mesh was created. The test copy remains an unsaved in-editor duplicate; Island, source graph, and pre-existing dirty shrub were not saved or modified. Resume only after the editor's existing dirty state is safe and the exception source is understood; use a smaller specimen and explicitly select standard Static Mesh for the HISM/PCG route.

**Post-restart follow-up (2026-10-11):** the tracked main project descriptor still enables both `ProceduralVegetationEditor` and `PlantFactoryPlugin`. The latter's local descriptor identifies it as a content-only mount for restored material functions, with no vendor executable modules. The plugin name being present therefore does not mean the retired PlantFactory application or a UE 5.8 engine integration is installed. Existing Festuca, Phalaris, and Typha material graphs resolve functions from this mount; removing it now would reintroduce missing material-function references. Keep it mounted until those materials are replaced or removed, while continuing to use UE 5.8 PVE as the separate vegetation-authoring tool. This follows the local material audit and [Epic's UE 5.8 PVE guide](https://dev.epicgames.com/documentation/unreal-engine/procedural-vegetation-editor-pve-in-unreal-engine), which documents both a standard Static Mesh export and Nanite Foliage export.

During a later read-only UI inventory, the initial app list still advertised the PVE graph window, but rehydrating that window failed because it had disappeared. The current window inventory at that point showed `dotnet.exe - Application Error` and no Unreal Editor window; the process query likewise returned no `UnrealEditor` process. This is consistent with the known intermittent editor/.NET failure but does not establish its cause or prove PVE triggered it. No window actions, exports, saves, or content changes occurred. Treat the prior dirty-editor warning as superseded only for current process status, not as evidence that the unsaved graph/shrub was saved. The static-mesh export remains unverified and should wait for the editor to be stable; do not relaunch or save over the user's project state as part of this note.

**Main-project Megaplant inventory refresh (2026-10-11):** a read-only
`UnrealEditor-Cmd` Python audit scanned 296 assets under
`/Game/Megaplant_Library` without asset-load errors. All 16 A/B/C/D tree
variants are `SkeletalMesh`; none is directly ready for the Island's HISM
scatter. Each tree uses two material slots. Bounds identify
`Tree_Common_Hazel_01_D` as the smallest output at about 89 cm tall (half-height
from `SkeletalMesh.GetBounds()`), followed by `Tree_Norway_Maple_Forest_01_D`
at 221 cm; the previous Hazel A candidate was about 480 cm tall. A–D are not
consistently ordered by height across species. The measured heights are:

| Species | A | B | C | D |
|---|---:|---:|---:|---:|
| Common Hazel | 480 cm | 292 cm | 174 cm | 89 cm |
| European Aspen | 1,413 cm | 1,023 cm | 717 cm | 451 cm |
| European Beech | 2,383 cm | 1,848 cm | 706 cm | 504 cm |
| Norway Maple | 1,223 cm | 1,379 cm | 565 cm | 221 cm |

The tested UE 5.8 Python wrapper does not expose SkeletalMesh LOD or bone
counts, so this inventory cannot prove that Hazel D is low-complexity. Its
bounds make it the better next export-dialog specimen, not an approved asset or
placement. The existing Hazel test copy was made from A and has no exported
mesh. During this refresh no PVE graph, Content asset, plugin setting, or Island
map was changed; the editor and Unreal MCP were not available, so do not launch
the unstable UI just to repeat the dialog. Logs:
[`initial inventory`](../../Saved/Playtests/Codex_MegaplantInventory_Followup_20261011/Logs/MegaplantInventory.log),
[`corrected bounds inventory`](../../Saved/Playtests/Codex_MegaplantInventory_Verify_20261011/Logs/MegaplantInventory.log).
The read-only inspector's vector and SkeletalMesh handling were corrected and
this final commandlet run completed successfully with the same 16/16 skeletal,
0 static-mesh result and no asset-load errors.

**PlantFactory content-mount update (2026-10-12):** the project-local
content-only `PlantFactoryPlugin` is now explicitly disabled in
`CaptiveSky_2.uproject`. Its two billboard functions were migrated to project
content, the three meadow master materials and generated Rhododendron repair
copy were rewired and recompiled, and a fresh UE 5.8.3 commandlet loaded Island
with zero PlantFactory assets mounted. The original plugin directory remains
for rollback; no PVE graph or Island placement was changed. This supersedes
earlier notes in this document saying to keep the PlantFactory mount enabled;
see [the migration finding](2026-10-12-plantfactory-migration.md). PVE remains
independently enabled and the static-mesh export task is still unverified.

**Installed PVE export API check (2026-10-12):** UE 5.8 exposes the export
settings on `UPVExportSettings`/`FPVExportParams`, including a standard
`StaticMesh` output mode, but the asset-baking implementation
(`FPVExporter` / `PV::Export::ExportCollectionAsMesh`) lives in the editor
module's `Private` source. I found no project-callable Python or Blueprint
export function in the installed plugin sources. That means setting the graph
node's export type alone is not an export; a commandlet route would have to
evaluate the copied graph through PCG and prove the export node performs the
bake. This route remains untested. No editor process was running during this
inspection, and no graph, asset, or map was changed. Prefer the existing
disposable graph's editor export dialog when the editor is stable; if pursuing
automation, first validate PCG graph execution and asset creation in a scratch
project, never on the source graph or Island.
