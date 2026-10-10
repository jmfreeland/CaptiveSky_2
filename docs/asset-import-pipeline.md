# ComfyBlender asset import

The generated props under `Saved/CaptiveSky/ComfyBlender/` are local, gitignored source packs. `Scripts/Import-ComfyBlenderAsset.py` imports one pack at a time into `/Game/Generated/ComfyBlender/<Asset>/`; it does not open, modify, or save the Island map. The resulting Unreal content is also local/ignored.

Run from PowerShell in the project root:

```powershell
$env:COMFYBLENDER_ASSET = 'NoticeBoard'
& 'D:\Games\Epic\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' `
  (Resolve-Path '.\CaptiveSky_2.uproject') `
  '-ExecutePythonScript=D:/Projects - Athena/Unreal/CaptiveSky_2/Scripts/Import-ComfyBlenderAsset.py' `
  -unattended -NoSplash -NoSound -NoZen -DDC-ForceMemoryCache
Remove-Item Env:\COMFYBLENDER_ASSET
```

Omit `COMFYBLENDER_ASSET` to select `NoticeBoard`, or use another folder name from `Saved/CaptiveSky/ComfyBlender/ASSETS.md`. The script only accepts simple alphanumeric/underscore names, resolves texture paths inside the generated library, imports the FBX with embedded collision and no automatic collision generation, and builds opaque materials from the manifest's BaseColor, Normal, ORM, scalar Roughness/Metallic, and Emissive entries. Existing same-named generated assets are replaced; unrelated content is left alone.

After import, inspect the `unreal_import.json` report in the selected source pack. It records imported paths, measured bounds in centimetres, material-slot assignments, and Unreal's collision-shape counts. A validation error is reported if dimensions differ from the source manifest by more than 1% (with a 2 cm floor), a manifest material slot is unassigned, or expected collision shapes are missing. This verifies import metadata, not visual quality: open the asset in the editor and inspect material appearance, collision in the Static Mesh Editor, lighting, and scale in a test scene before placing anything on the Island map.

The report is local and ignored with the generated pack. Keep the source script and manifest as the editable/golden source; do not hand-edit imported meshes as a substitute. Only place the asset after its appearance, scale, and collision have been reviewed.

As of 2026-10-10, the NoticeBoard and StoneLantern are imported under `/Game/Generated/ComfyBlender/`. Their reports match expected scale and material slots; the embedded collisions are one box and one convex hull respectively. The StoneLantern emissive slot is assigned, but both props still need an in-editor lighting/material review before either is placed on the Island.

## Fab cache check — Megaplants

The local Fab cache currently contains four species families under `D:\Projects - Athena\Unreal\Fab_Cache\VaultCache\FabLibrary\`: Common Hazel, European Aspen, European Beech, and Norway Maple Forest. Each has four A–D tree variants and four Dynamic Wind JSON sidecars. Hazel and Aspen include a `Foliage.usd`; Beech and Norway Maple include a `Branches.usd`. The cache contains USD files and listing metadata/thumbnails only—no Unreal `.uasset` files, FBX files, or separate image textures. The USD files are valid binary USD crates, but their presence does not establish that all material dependencies or the authored Unreal setup are available.

Each cached Fab listing advertises both USD and Unreal Engine formats, but the selected/cached format is `usd`; only the Aspen metadata explicitly records target `UE_5.8`. Prefer downloading the Unreal Engine format for UE 5.8 if available in the Fab Vault, then validate a representative tree and its wind setup before placement. Otherwise, test the USD and its dependencies in an isolated import location before adding anything to `/Game` or the Island map. This inventory was read-only; no cache files or Unreal assets were changed.

**UE 5.8.3 USD import smoke test (2026-10-10):** in a disposable content-only probe project under `Saved/CompileScratch/FabUsdProbe_20261010/`, both European Aspen `Tree_European_Aspen_01_A.usd` and `Tree_European_Aspen_01_Foliage.usd` imported successfully through the USD Stage importer. The tree file generated 45 assets (15 skeletal meshes, 14 skeletons, 14 physics assets, and 2 material instances); the foliage file generated 20 (6 of each skeletal mesh/skeleton/physics asset and 2 material instances). Neither produced a `StaticMesh` or a separate texture asset. This proves the USD can be converted by UE 5.8.3, but it is **not a drop-in source for the Island's current HISM vegetation scatter**, which consumes static meshes. The Dynamic Wind JSON sidecar was not applied or runtime-tested. Use the native Unreal Fab format if possible; otherwise this would need a separate, visually validated skeletal-vegetation integration before it could contribute to the Island. Logs: `Saved/CompileScratch/FabUsdProbe_20261010/Saved/Logs/FabUsdProbe.log`.

## Resident proposals

Residents can use `request_upgrade` for any existing world object or feature, not just props produced by this pipeline: vegetation (including a specific instance), structures, paths, water, lighting, sound, or a broader level feature. A target can be omitted for a genuinely world-wide suggestion. Proposals are appended to `Saved/CaptiveSky/ComfyBlender/Requests/inbox.jsonl` for human review; the action does not edit an asset, run a generator, import content, or place anything in the level. The proposal's `target`, `upgrade_kind` (`aesthetic`, `variation`, or `functionality`), and concise description are carried through so a human can decide how to fulfill it—via this asset pipeline, a code change, an authored Unreal asset, or no change.

`CaptiveSky2.Agent.AssetRequestQueue` now exercises a structure target, a specific foliage-instance target, and a targetless world-level proposal. The focused Unreal 5.8.3 automation test passed on 2026-10-10.
