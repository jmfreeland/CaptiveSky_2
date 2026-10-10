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
