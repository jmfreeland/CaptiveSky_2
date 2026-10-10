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

## Resident proposals

Residents can use `request_upgrade` for any existing world object or feature, not just props produced by this pipeline: vegetation (including a specific instance), structures, paths, water, lighting, sound, or a broader level feature. A target can be omitted for a genuinely world-wide suggestion. Proposals are appended to `Saved/CaptiveSky/ComfyBlender/Requests/inbox.jsonl` for human review; the action does not edit an asset, run a generator, import content, or place anything in the level. The proposal's `target`, `upgrade_kind` (`aesthetic`, `variation`, or `functionality`), and concise description are carried through so a human can decide how to fulfill it—via this asset pipeline, a code change, an authored Unreal asset, or no change.

`CaptiveSky2.Agent.AssetRequestQueue` now exercises a structure target, a specific foliage-instance target, and a targetless world-level proposal. The focused Unreal 5.8.3 automation test passed on 2026-10-10.
