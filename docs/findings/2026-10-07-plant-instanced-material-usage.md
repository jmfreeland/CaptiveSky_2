# Plant instanced-material usage repair (2026-10-07)

## Audit

Standalone Island startup warned that plant material instances were missing the `InstancedStaticMeshes` usage flag and would use the default material. The updated read-only `Scripts/Inspect-IslandPlantMaterials.py` checks that flag along each mesh material's parent chain and separately checks material-instance overrides.

Before the repair, the UE 5.8.3 audit found 38 material-chain assets without the usage flag. The missing flag originated in four base materials: Festuca, Phalaris, Typha, and Rhododendron. None of the inspected material instances overrides this usage. The Rhododendron master still has four unresolved material-function references in its graph, as documented in [the PlantFactory dependency audit](2026-10-04-plant-material-dependencies.md), so it was deliberately excluded from the repair.

## Change and verification

The idempotent `Scripts/Enable-IslandPlantInstancedMaterials.py` enables `MATUSAGE_InstancedStaticMeshes` on the three graph-valid base materials, recompiles them, verifies the flag, and saves only after all three compile successfully. UE 5.8.3 reported successful recompile/save and a post-save flag check for each material.

The post-save read-only audit reported:

- Missing usage flags: **38 → 15**; the remaining 15 assets are all in the deferred Rhododendron family.
- Reference issues: **4 → 4**, unchanged; no audit API errors.
- No material instance needed a usage override; the three repaired parent flags now propagate through their instance chains.

The exact four original base-material packages were backed up before editing under `Saved/Playtests/Codex_PlantInstancedUsage_20261007/AssetBackup/`. SHA-256 values (backup → current):

- Festuca: `FEAA538F4C6AB82A5E31FCEDAFCA41303EA8AAC0D0B7D6038EDB191CC4690700` → `E54F22FDD1A361FBDDCA9B3F0F0FA8501CCF5E580255CF81A40305AA9E74FBA5`
- Phalaris: `930ABEA81BC79EB2C2A4C5B9CD7F1B5C01C095095EFC7DBC0E82BAC53A35BAC1` → `21CCD6D089C1F5D32F48DD37495ABE5315B6F01AD85A1C4B4070FB6FC34BA953`
- Typha: `870B287F64E175D4450B22879272BBBE0DBC4621C828238C775E8EA465779A96` → `4888BB32BFA1F80969A5BB5DD740823558716225C5E772CACAF78F2C944D006C`
- Rhododendron, intentionally unchanged: `2B1746766E5EC27371AFDC7E58B7996A314320161A5CF943792A0D9CBCE055AB` → same

These assets are ignored local Content files and are not distributed by this Git commit. The three repaired masters are Festuca, Phalaris, and Typha; the Rhododendron source package is preserved in the backup but was not edited.

## Runtime limit and next step

A bounded 120-second standalone render check was attempted with agent thinking and Python disabled. UE remained at `TurnkeySupport` before world initialization for over a minute, with no log progress or screenshots. Only the run's verified UnrealEditor PID and its idle UBT child were stopped. Therefore, the post-save usage flags are verified through the asset audit, but their visual effect in a fully loaded game is **not yet verified**. Log: `Saved/Logs/Codex_PlantUsageRender_20261007.log`.

When a normal interactive editor/game launch is available, recapture Tideglass and verify the three repaired plant families render without default-material warnings. Keep Rhododendron deferred until its missing material-function references can be repaired from source evidence rather than guessed graph edits.

API reference: [UE 5.8 MaterialEditingLibrary Python API](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/MaterialEditingLibrary).
