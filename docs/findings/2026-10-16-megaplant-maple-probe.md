# Norway Maple Megaplant preview

## Result

The installed Norway Maple skeletal meshes are loadable, but the current project configuration does not yet give a trustworthy foliage-quality preview. The bounded, transient preview showed the A variant as a bare branching structure and the D variant as a much sparser sapling. Neither should be scattered into the Island on this evidence.

The UE 5.8.3 log for both variants warns that building the Nanite Assembly failed because Nanite Foliage is not enabled. The project currently sets `r.Nanite.Foliage=False` in `Config/DefaultEngine.ini`; UE marks this setting experimental and restart-required in `RendererSettings.h`. Attempting to change `r.Nanite.AllowAssemblies` through `-ExecCmds` failed because that CVar is read-only. This means the bare captures may reflect the disabled assembly path rather than the authored Megaplant appearance.

## Probe details

- Previewed `Tree_Norway_Maple_Forest_01_A` and `_D` at `Roost_West` through `Scripts/Preview-InstancedSkinnedMegaplant.py` in the isolated `Saved/CompileScratch/Claude_Props` project.
- Each run spawned exactly one transient instance and one ordinary skeletal-mesh comparison, captured two images, and exited without saving the Island or any asset. Character thinking was disabled and model requests were capped at zero.
- A third A-variant run passed `r.Nanite.AllowAssemblies 1` with `-ExecCmds`; UE rejected it as read-only, so it did not test Nanite assemblies.
- Captures/logs are disposable evidence under `Saved/CompileScratch/Claude_Props/Saved/Playtests/Codex_NorwayMaple{A,D}*_20261016` and `Saved/CompileScratch/Claude_Props/Saved/Logs/Codex_NorwayMaple*20261016.log`.
- The primary Unreal editor was left running as requested; no UI interaction, process termination, project setting change, map edit, or asset save was performed.

## Next gate

If revisiting this asset, make a scratch-project-only, restart-time override for the experimental Nanite Foliage/assembly support, then recapture one variant. Do not change the main project setting or scatter these meshes until the canopy is visibly correct and runtime performance has been measured. Existing Island profiling warns that experimental Nanite Foliage has been costly for the current dense ground-cover workload; that result does not quantify this tree asset, but it argues against enabling the feature globally without a paired runtime profile.
