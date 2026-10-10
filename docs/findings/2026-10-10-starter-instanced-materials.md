# StarterContent instanced-material usage repair (2026-10-10)

## Finding and change

The bounded Tideglass Game log reported that StarterContent `M_Rock` and
`M_Wood_Oak` lack the `InstancedStaticMeshes` usage permutation, so affected
HISM instances fall back to Unreal's Default Material. A non-saving UE 5.8.3
probe enabled the permutation in memory and compiled both base materials with
no compile errors. The tracked
[`Enable-IslandStarterInstancedMaterials.py`](../../Scripts/Enable-IslandStarterInstancedMaterials.py)
then backed up the two package files, recompiled both successfully, and saved
only those materials. The script does not save the Island map.

| Material | Usage before | Usage after | Original SHA-256 | Saved SHA-256 |
|---|---:|---:|---|---|
| `/Game/StarterContent/Props/Materials/M_Rock` | false | true | `2A2A237CFC23C8D44BB96B71CCA8FE3F2DA4BCF37BF03C4890CB1788E2EA3DD3` | `8303A672B5A657E42B10CBFBD5779525260262050A90F0E8A21BEE46289FC277` |
| `/Game/StarterContent/Materials/M_Wood_Oak` | false | true | `DAD76CD1109C8B4A3594E23DD83351170764980C3CCFE57859A3A2F60FEF5CDD` | `C67D3B922B6530DF06AB9DDE97EB1BBB4A0C85C683570E5B9DC740EB31EB6154` |

Exact original package backups are in the ignored local folder
`Saved/AssetBackups/Codex_StarterInstancedMaterials_20261010/`; their hashes
were checked against the pre-edit files. The binary `Content` assets remain
local and are not part of the Git commit.

## Verification and limits

- The in-memory compile probe passed for both materials without saving any
  packages: `Saved/Logs/Codex_StarterInstancedUsageProbe_20261010.log`.
- The guarded save run reported `usage_after: True` for both, with the hashes
  above: `Saved/Logs/Codex_StarterInstancedUsage_20261010.log`.
- No post-repair Game screenshot or runtime log has been captured yet, so the
  disappearance of the two warnings and the visual effect are not yet verified.
  A live UnrealEditor process was present after the save run; no second session
  was started alongside it.
- The 13 Everestian rhododendron material fallbacks are a separate unresolved
  graph issue and were not changed; see
  [the Rhododendron finding](2026-10-08-rhododendron-material-fallback.md).

Next: after the editor is closed, recapture the same bounded Tideglass Game
view with agent thinking disabled and a zero-request cap. Confirm the two
StarterContent warnings are gone, inspect whether the rock/wood HISM instances
now use their authored materials, and keep the unresolved Rhododendron fallback
separate from this result.
