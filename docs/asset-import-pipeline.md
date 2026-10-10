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

Each cached listing's local metadata advertises USD and Unreal Engine format types, but the selected/cached format is `usd`; only the Aspen metadata explicitly records target `UE_5.8`. There is a format-availability ambiguity: the current public Fab pages show USD under “Included formats,” while Fab documentation says the listing's download format can be selected. I could not verify the signed-in Launcher's selector, so treat a native Unreal package as **unconfirmed** until it is explicitly selected and downloaded; check for `.uasset` files before planning a native import. The four listings currently show as free. If an Unreal Engine package is offered, validate a representative tree and its wind setup before placement. Otherwise, test the USD and its dependencies in an isolated import location before adding anything to `/Game` or the Island map. This inventory was read-only; no cache files or Unreal assets were changed. Sources: [European Aspen listing](https://www.fab.com/listings/ffa90e1a-e420-43d6-ade3-daa4bc189a0a), [Fab export documentation](https://dev.epicgames.com/documentation/en-us/fab/exporting-assets-from-fab-in-launcher).

**Procedural Vegetation Editor status:** the UE 5.8.3 engine installation includes its Experimental plugin and editor binary, but the plugin descriptor marks it experimental and disabled by default. `CaptiveSky_2.uproject` and `Config/` do not enable or configure it, and no project-content paths named for Megaplants, Procedural Vegetation, or Dynamic Wind were found. The plugin depends on Dataflow, GeometryScripting, PCG, and DynamicWind. None were enabled or tested for this project in this check. The cached USD import test therefore remains a USD-to-skeletal-mesh conversion, not a PVE graph/preset workflow; enabling the plugin alone is not a demonstrated bridge to the Island's HISM static-mesh scatter.

**UE 5.8.3 USD import smoke tests (2026-10-10):** in disposable content-only probe project `Saved/CompileScratch/FabUsdProbe_20261010/`, European Aspen `Tree_European_Aspen_01_A.usd` and `Tree_European_Aspen_01_Foliage.usd` imported through the USD Stage importer. The tree generated 45 assets (15 skeletal meshes, 14 skeletons, 14 physics assets, and 2 material instances); the foliage file generated 20 (6 each of skeletal meshes, skeletons, and physics assets, plus 2 material instances). A follow-up imported one European Beech A and one Norway Maple A variant: Beech generated 51 assets including 17 skeletal meshes; Maple generated 27 including 9 skeletal meshes. **All four imports produced zero `StaticMesh` and zero separate texture assets.** Thus the cached USDs are valid and importable, but are **not drop-in sources for the Island's HISM vegetation scatter**, which consumes static meshes. The Dynamic Wind JSON sidecars were not applied or runtime-tested; the imports do not validate PVE or in-world appearance/performance. Prefer obtaining and testing the native Unreal Fab package if available. Otherwise, any conversion or skeletal-vegetation integration needs its own visual and performance validation before contributing to the Island. Aspen logs: `Saved/CompileScratch/FabUsdProbe_20261010/Saved/Logs/FabUsdProbe.log`; Beech/Maple log: `Saved/CompileScratch/FabUsdProbe_20261010/Saved/Logs/FabUsdMultiProbe.log`.

## Resident proposals

Residents can use `request_upgrade` for any existing world object or feature, not just props produced by this pipeline: vegetation (including a specific instance), structures, paths, water, lighting, sound, or a broader level feature. A target can be omitted for a genuinely world-wide suggestion. Proposals are appended to `Saved/CaptiveSky/ComfyBlender/Requests/inbox.jsonl` for human review; the action does not edit an asset, run a generator, import content, or place anything in the level. The proposal's `target`, `upgrade_kind` (`aesthetic`, `variation`, or `functionality`), and concise description are carried through so a human can decide how to fulfill it—via this asset pipeline, a code change, an authored Unreal asset, or no change.

Review resident proposals from the project root with:

```powershell
python -I Scripts/AssetPipeline/requests.py list
python -I Scripts/AssetPipeline/requests.py list --all
python -I Scripts/AssetPipeline/requests.py status <request-id> accepted
```

`accepted` only records the human decision; it does not invoke ComfyUI, Blender, or Unreal. For an accepted `new_object`, create or adapt a reviewed script under `Scripts/AssetPipeline/assets/`, build it with `make.py`, inspect its exported preview and manifest, and import it separately using the procedure above. For an `upgrade`, decide whether to use this asset pipeline, change code, author an Unreal asset, or decline the proposal; upgrades are not limited to pipeline-generated props. Mark the request `generated` only after the result is actually made, or `declined` if it will not be pursued. The inbox remains a review queue, not an autonomous asset-generation command channel.

`CaptiveSky2.Agent.AssetRequestQueue` now exercises a structure target, a specific foliage-instance target, and a targetless world-level proposal. The focused Unreal 5.8.3 automation test passed on 2026-10-10.
