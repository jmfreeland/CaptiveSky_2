# Tideglass water

The first rendered Tideglass view showed a nearly featureless white flattened sphere. A preview of
the existing ocean material on that shallow mesh rendered almost black, so it is reserved for the
open ocean. The pool instead uses a separate opaque teal material with two world-aligned animated
normal swells, a soft edge reflection, and weather-driven roughness/normal strength from
`MPC_IslandEnvironment` (`WindSpeed`, `Storm`, and `RainIntensity`). This is a visual prototype, not
a depth, temperature, water-quality, or swimming simulation.

`Scripts/Create-TideglassPoolMaterial.py` creates `/Game/Materials/M_TideglassPool_NormalizedWind`
and refuses to overwrite that output. The older `/Game/Materials/M_TideglassPool` is preserved; the
generated `.uasset` is in the ignored `Content/` tree and is not versioned here. Run the script
headlessly with UE 5.8.3 `-ExecutePythonScript` when the new asset is missing.
`UIslandTideglassSubsystem` finds the flattened sphere beside the `TideglassPool` marker,
applies the material in Game/PIE only, and restores the authored component material at world teardown.
The saved Island map and its material assignment are not changed; without the generated asset, the
subsystem leaves the white placeholder alone.

For an editor-world-only rendered comparison, run:

```powershell
./Scripts/Capture-Viewpoints.ps1 -Hour 12 -Only Tideglass -NoWorldState -LogPath Saved/Logs/TideglassBaseline.log
./Scripts/Capture-Viewpoints.ps1 -Hour 12 -Only Tideglass -TideglassMaterial /Game/Materials/M_TideglassPool_NormalizedWind -NoWorldState -LogPath Saved/Logs/TideglassTeal.log
./Scripts/Capture-Viewpoints.ps1 -Hour 12 -Only Tideglass -TideglassMaterial /Game/Materials/M_TideglassPool_NormalizedWind -TideglassWeather Calm -NoWorldState
./Scripts/Capture-Viewpoints.ps1 -Hour 12 -Only Tideglass -TideglassMaterial /Game/Materials/M_TideglassPool_NormalizedWind -TideglassWeather Storm -NoWorldState
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

`-TideglassWeather Calm` or `-TideglassWeather Storm` holds the actual `MPC_IslandEnvironment` wind/storm/rain scalars at
known diagnostic values for the render, then restores their captured values. This tests the material
response, not the weather actor or gameplay integration. The response now normalizes wind measured in
cm/s against a tunable 300 cm/s reference before blending, avoiding the previous immediate saturation
at ordinary breeze speeds. Matched UE 5.8.3 noon captures succeeded after rebuild, and the test now
asserts the original collection values were restored. Calm keeps a concentrated specular highlight;
the forced storm softens that highlight and darkens the pool, though the difference remains restrained
on the flattened-sphere blockout. The 17:00 golden-hour render carries a warm-to-cool split reflection,
while 22:00 keeps the pool luminous and legible against the dark shore. Captures are under
`Saved/Viewpoints/2026-10-01_073931_h12.0/` (calm) and
`Saved/Viewpoints/2026-10-01_074022_h12.0/` (storm); logs are
`Saved/Logs/Codex_TideglassCalmVerified_20261001.log` and
`Saved/Logs/Codex_TideglassStormVerified_20261001.log`. Lighting captures are under
`Saved/Viewpoints/2026-10-01_074518_h17.0/` (golden hour) and
`Saved/Viewpoints/2026-10-01_074408_h22.0/` (night); logs are
`Saved/Logs/Codex_TideglassGoldenHour_20261001.log` and
`Saved/Logs/Codex_TideglassNight_20261001.log`.
The surface is still a flattened sphere, so its disc-like silhouette remains a blockout; check a real
pool mesh and better shoreline integration before treating this as final art. Keep the map material
reversible and do not overwrite the authored sphere material.
