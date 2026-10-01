# Tideglass water

The first rendered Tideglass view showed a nearly featureless white flattened sphere. A preview of
the existing ocean material on that shallow mesh rendered almost black, so it is reserved for the
open ocean. The pool instead uses a separate opaque teal material with two world-aligned animated
normal swells, a soft edge reflection, and weather-driven roughness/normal strength from
`MPC_IslandEnvironment` (`WindSpeed`, `Storm`, and `RainIntensity`). This is a visual prototype, not
a depth, temperature, water-quality, or swimming simulation.

`Scripts/Create-TideglassPoolMaterial.py` creates `/Game/Materials/M_TideglassPool` and refuses to
overwrite an existing asset. The generated `.uasset` is in the ignored `Content/` tree and is not
versioned here. Run the script headlessly with UE 5.8.3 `-ExecutePythonScript` when the asset is
missing. `UIslandTideglassSubsystem` finds the flattened sphere beside the `TideglassPool` marker,
applies the material in Game/PIE only, and restores the authored component material at world teardown.
The saved Island map and its material assignment are not changed; without the generated asset, the
subsystem leaves the white placeholder alone.

For an editor-world-only rendered comparison, run:

```powershell
./Scripts/Capture-Viewpoints.ps1 -Hour 12 -Only Tideglass -NoWorldState -LogPath Saved/Logs/TideglassBaseline.log
./Scripts/Capture-Viewpoints.ps1 -Hour 12 -Only Tideglass -TideglassMaterial /Game/Materials/M_TideglassPool -NoWorldState -LogPath Saved/Logs/TideglassTeal.log
```

The capture applies the requested material only to the target component and restores its original
slot after rendering. A matched October 1, 2026 noon pair confirmed the blockout's white disc became
legible teal water. The rejected `M_IslandOcean` preview is also recorded in
`Saved/Logs/Codex_TideglassWaterPreview_MIslandOcean_20261001.log`; baseline and teal captures are
under `Saved/Viewpoints/2026-10-01_065235_h12.0/` and
`Saved/Viewpoints/2026-10-01_070300_h12.0/`.

UE 5.8.3 validation completed:

- `CaptiveSky2.Agent.IslandTideglass` passed on the saved Island, checking the real target finder,
  Game-world start application, reversible restoration, and the synthetic fixture.
- `CaptiveSky2.Visual.Viewpoints` passed for the baseline and material preview.
- A provider-free Game probe applied the runtime material, completed the Innkeeper's 959 cm route,
  and ended after 11.3 real seconds with zero model requests. It used
  `Saved/Playtests/Codex_TideglassRuntime_20261001` for isolated data.

The color and weather response have not yet been compared across clear, heavy-rain, dusk, and night
renders. The surface is still a flattened sphere, so its disc-like silhouette remains a blockout;
check a real pool mesh and better shoreline integration before treating this as final art. Keep the
map material reversible and do not overwrite the authored sphere material.
