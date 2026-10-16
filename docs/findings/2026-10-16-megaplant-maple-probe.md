# Norway Maple Megaplant preview

## Result

The installed Norway Maple skeletal meshes are loadable, but neither tested variant is ready to place. With the project's default setting, A appeared bare and D was a sparse sapling. Enabling Nanite Foliage only in the scratch process reveals leaf geometry on A, but the foliage remains gray in the base-color pass and almost black in final color, including at a verified noon capture. A Beech A control renders green in the same base-color mode, so this is Maple-specific rather than a general preview/light-level issue.

The UE 5.8.3 log for the default-setting runs warns that building the Nanite Assembly failed because Nanite Foliage is not enabled. The project sets `r.Nanite.Foliage=False` in `Config/DefaultEngine.ini`; UE marks the feature experimental and restart-required in `RendererSettings.h`. A scratch-only `-ini:Engine:[/Script/Engine.RendererSettings]:r.Nanite.Foliage=True` override was confirmed in the log (`Set CVar [[r.Nanite.Foliage:1]]`) and removed the assembly warning. The rejected `-ExecCmds` attempt to set `r.Nanite.AllowAssemblies` is read-only and does not enable the feature.

## Probe details

- Previewed `Tree_Norway_Maple_Forest_01_A` and `_D` at `Roost_West` through `Scripts/Preview-InstancedSkinnedMegaplant.py` in the isolated `Saved/CompileScratch/Claude_Props` project.
- Each run spawned exactly one transient instance and one ordinary skeletal-mesh comparison, captured two images, and exited without saving the Island or any asset. Character thinking was disabled and model requests were capped at zero.
- A third A-variant run passed `r.Nanite.AllowAssemblies 1` with `-ExecCmds`; UE rejected it as read-only, so it did not test Nanite assemblies.
- The preview helper now has separate final-color and base-color capture modes, a verified transient capture hour, an optional process-only sun-intensity override, and a Skeletal Mesh LOD section/material-slot report. It explicitly assigns the transient dynamic material and records parameter readbacks. For Maple, the tint values read back as green for the common and leaf-specific parameter names, but the base-color capture remains neutral gray; LOD0 references both material slots `[0, 1]`. Beech A responds green in the same base-color path. This points to a Maple-specific material/assembly binding or shader issue, not a failed preview assignment or a general lighting problem.
- A noon Maple capture reports the project's unmodified sun intensity as 7.6 (`[-90, 35, 0]` rotation). A transient intensity of 25 still leaves the tree dark, while 120,000 clips the fixed-exposure LDR capture to white; do not use these diagnostics to infer or change the game's sun settings.
- Captures/logs are disposable evidence under `Saved/CompileScratch/Claude_Props/Saved/Playtests/Codex_NorwayMaple*20261016` and `Saved/CompileScratch/Claude_Props/Saved/Logs/Codex_NorwayMaple*20261016.log`; the Beech control uses `Codex_BeechA_BaseColorControl_20261016`.
- Captures/logs are disposable evidence under `Saved/CompileScratch/Claude_Props/Saved/Playtests/Codex_NorwayMaple{A,D}*_20261016` and `Saved/CompileScratch/Claude_Props/Saved/Logs/Codex_NorwayMaple*20261016.log`.
- No UI interaction or process termination was issued. At the final read-only process check, PID 22688 and all `UnrealEditor*` processes no longer resolved. The main project log had no explicit crash or clean-shutdown record; the latest Island autosave was `Saved/Autosaves/Game/Maps/Island_Auto1.umap` at 11:07 on 2026-10-16. Why the editor process disappeared is unknown, and unsaved edits after that autosave are not confirmed preserved. The Island was not saved or restored by these preview runs.

## Next gate

Next, inspect the Maple Skeletal Mesh's LOD section bindings and the `MI_Norway_Maple_Foliage_01` / `MA_Foliage_Trees` shader path against Beech's working material. Keep the investigation scratch-only; do not change the main project setting or scatter Maple until leaf color/silhouette is correct and runtime performance is measured. Existing Island profiling warns that experimental Nanite Foliage has been costly for dense ground cover; that does not quantify this tree asset, but argues against enabling the feature globally without a paired runtime profile.
