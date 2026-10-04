# PlantFactory material dependencies: audit and original-content restore

## Evidence and decision

Recent Island captures logged missing `/PlantFactoryPlugin/StaticBillboard` and `/PlantFactoryPlugin/BillboardMappedNormals` packages. The old PlantFactory project enables that plugin, but its functions were not mounted in CaptiveSky_2.

`Scripts/Inspect-IslandPlantMaterials.py` provides a read-only audit of mesh LOD section material slots, instance parent chains, static switches, and nested material-function calls. The script does not load or save a map, modify assets, compile shaders, start gameplay, or make external calls. Its JSON report and explicit completion/issue count are written to the editor log; a completed audit is not a clean audit unless its issue count is zero.

Before restoration (`Saved/Logs/Codex_PlantMaterialAudit_20261004.log`), four PlantFactory master graphs each contained four unresolved function calls: Festuca, Phalaris, Typha and the original rhododendron. Sixteen missing calls in total; all inspected meshes/material slots resolved.

The original function assets were found locally in `C:/Program Files/e-on software/PlantFactory 2024/plugins/Unreal/Content`. Rather than replace them with guessed shader math, a content-only project plugin mounts those originals at their expected `/PlantFactoryPlugin` path. No vendor executable module is included, and existing mesh/material/map assets are not changed.

## Local installation and source control

Run `Scripts/Restore-PlantFactoryContent.ps1` with your own licensed installation. Its default source is the local PlantFactory 2024 installation; `-SourceContent` permits another installed original function-content directory. `-ProjectDir` supports an isolated CaptiveSky_2 scratch project.

The script requires both missing function files, preflights target hashes, and refuses differing existing assets. It copies the original directory's 16 flat `.uasset` function files, not executables. The descriptor and scripts are tracked; `Plugins/PlantFactoryPlugin/Content/` is ignored, like the main project Content. These vendor assets must be restored locally or included in the project's external asset backup, not redistributed through this repository.

Validation: installed first in scratch, then in the primary checkout; a repeated install succeeded. `git check-ignore` confirmed binary exclusion. A deliberate differing-asset fixture under `Saved/PlantRestoreSafety_20261004` was rejected without changing its hash. The UE 5.8.3 Development Editor build succeeded with the content-only plugin present.

## Reference results and remaining limits

After restoration (`Saved/Logs/Codex_PlantMaterialAuditRestored_20261004.log`), the plugin mounted successfully and the missing package warnings disappeared. Festuca, Phalaris and Typha now have resolved function references. **Four null calls remain in the original rhododendron master**; restoring the package does not recover those already-null graph references. Do not rewire them without original graph evidence or a reviewed replacement. The primary checkout audit independently confirmed the same mounted plugin and four remaining issues (`Saved/Logs/Codex_PlantMaterialAuditPrimary_20261004.log`).

All four imported PlantFactory meshes have only LOD 0. Their material instances report `Billboard material=false`; this is evidence that billboard mode is disabled, not proof of shader reachability or rendered material correctness. The currently imported spruce has five LODs, with the final LOD using its imposter material; its inspected nested function references resolved.

Consequently, the missing billboard packages were an import-integrity problem, but are not proven to explain the sparse middle-distance vegetation or thin tree silhouettes. Neither the existing slot-count fixture nor this reference audit proves complete visual/shader health. The one-LOD PlantFactory meshes also do not provide a ready-made distant vegetation strategy.

The audit uses Epic's documented [material-expression and static-switch inspection APIs](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/MaterialEditingLibrary); LOD slot inspection was checked against the installed UE 5.8 StaticMeshEditorSubsystem header.

## Next

The 11:00 Wind Arch capture rendered successfully to scratch `Saved/Viewpoints/2026-10-04_151330_h11.0/04_WindArchOverlook.png`, without the earlier missing PlantFactory package warnings or an obvious visual fallback. However, its **automation timing gate failed** (`Saved/Logs/Codex_PlantRestoreWindArch_20261004.log`): 11.51 FPS p95 over 49/50 valid intervals and 8.90 wall FPS. The log includes repeated multi-second deltas and a connectivity request timeout. This is not a performance clearance or proof that the restore caused (or did not cause) the timing problem. A reference repair and successful build do not waive the 30 FPS gate.

Investigate the rhododendron's null calls separately, and profile the repeated first-view rendering stalls with warmed, controlled comparisons. Continue toward actual woodland silhouettes and semantic habitat composition; do not grow the global instance count to compensate for missing LOD/art direction.
