# Norway Maple Megaplant preview

## Result

The installed Norway Maple skeletal meshes are loadable, but neither tested variant is ready to place. With the project's default setting, A appeared bare and D was a sparse sapling. Enabling Nanite Foliage only in the scratch process revealed leaf geometry on A, but the canopy remained nearly black and blocky even in a verified noon capture. This is not yet a trustworthy quality result for the asset's intended appearance.

The UE 5.8.3 log for the default-setting runs warns that building the Nanite Assembly failed because Nanite Foliage is not enabled. The project sets `r.Nanite.Foliage=False` in `Config/DefaultEngine.ini`; UE marks the feature experimental and restart-required in `RendererSettings.h`. A scratch-only `-ini:Engine:[/Script/Engine.RendererSettings]:r.Nanite.Foliage=True` override was confirmed in the log (`Set CVar [[r.Nanite.Foliage:1]]`) and removed the assembly warning. The rejected `-ExecCmds` attempt to set `r.Nanite.AllowAssemblies` is read-only and does not enable the feature.

## Probe details

- Previewed `Tree_Norway_Maple_Forest_01_A` and `_D` at `Roost_West` through `Scripts/Preview-InstancedSkinnedMegaplant.py` in the isolated `Saved/CompileScratch/Claude_Props` project.
- Each run spawned exactly one transient instance and one ordinary skeletal-mesh comparison, captured two images, and exited without saving the Island or any asset. Character thinking was disabled and model requests were capped at zero.
- A third A-variant run passed `r.Nanite.AllowAssemblies 1` with `-ExecCmds`; UE rejected it as read-only, so it did not test Nanite assemblies.
- The preview helper now applies tint to the foliage material's actual `BaseColor Tint Leaf Backside` and `BaseColor Tint Leaves` parameters, and supports a verified transient capture hour. The noon run reports hour 12.0, sun intensity 7.6 and rotation `[-90, 35, 0]`; despite the applied green tint, the captured leaves remain almost black. This narrows the problem to material/lighting/render behavior, not missing Nanite assemblies or the old mismatched tint parameter name.
- Captures/logs are disposable evidence under `Saved/CompileScratch/Claude_Props/Saved/Playtests/Codex_NorwayMaple{A,D}*_20261016` and `Saved/CompileScratch/Claude_Props/Saved/Logs/Codex_NorwayMaple*20261016.log`.
- No UI interaction or process termination was issued. At the final read-only process check, PID 22688 and all `UnrealEditor*` processes no longer resolved. The main project log had no explicit crash or clean-shutdown record; the latest Island autosave was `Saved/Autosaves/Game/Maps/Island_Auto1.umap` at 11:07 on 2026-10-16. Why the editor process disappeared is unknown, and unsaved edits after that autosave are not confirmed preserved. The Island was not saved or restored by these preview runs.

## Next gate

Next, diagnose why the correct material parameters still render black (including the imported master material dependency and light/exposure path) with scratch-only captures. Do not change the main project setting or scatter these meshes until the canopy is visibly correct and runtime performance has been measured. Existing Island profiling warns that experimental Nanite Foliage has been costly for the dense ground-cover workload; that does not quantify this tree asset, but argues against enabling the feature globally without a paired runtime profile.
