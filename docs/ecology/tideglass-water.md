# Tideglass water

The first rendered Tideglass view showed a nearly featureless white flattened sphere. A preview of
the existing ocean material on that shallow mesh rendered almost black, so it is reserved for the
open ocean. At Game/PIE start the subsystem now uses the saved sphere only as a reversible footprint:
it creates a 64-segment procedural surface with two shallow rings and a softened irregular outline,
then hides the sphere until teardown. The generated top has no collision; the original footprint's
collision remains in place. It uses a separate opaque teal material with two world-aligned animated
normal swells, a soft edge reflection, and weather-driven roughness/normal strength from
`MPC_IslandEnvironment` (`WindSpeed`, `Storm`, and `RainIntensity`). The Island map and blockout
component are never saved with runtime changes. This is a visual prototype, not a depth, temperature,
water-quality, or swimming simulation.

`Scripts/Create-TideglassPoolMaterial.py` creates `/Game/Materials/M_TideglassPool_NormalizedWind`
and refuses to overwrite that output. The older `/Game/Materials/M_TideglassPool` is preserved; the
generated `.uasset` is in the ignored `Content/` tree and is not versioned here. Run the script
headlessly with UE 5.8.3 `-ExecutePythonScript` when the new asset is missing.
`UIslandTideglassSubsystem` finds the flattened sphere beside the `TideglassPool` marker and generates
the runtime-only surface over its bounds. It restores the sphere's prior visibility and destroys the
procedural component at world teardown. Without the generated material asset, it leaves the original
white placeholder visible.

For an editor-world-only rendered comparison, run:

```powershell
./Scripts/Capture-Viewpoints.ps1 -Hour 12 -Only Tideglass -NoWorldState -LogPath Saved/Logs/TideglassBaseline.log
./Scripts/Capture-Viewpoints.ps1 -Hour 12 -Only Tideglass -TideglassMaterial /Game/Materials/M_TideglassPool_NormalizedWind -NoWorldState -LogPath Saved/Logs/TideglassTeal.log
./Scripts/Capture-Viewpoints.ps1 -Hour 12 -Only Tideglass -TideglassMaterial /Game/Materials/M_TideglassPool_NormalizedWind -TideglassWeather Calm -NoWorldState
./Scripts/Capture-Viewpoints.ps1 -Hour 12 -Only Tideglass -TideglassMaterial /Game/Materials/M_TideglassPool_NormalizedWind -TideglassWeather Storm -NoWorldState
```

The material preview uses the same transient procedural geometry and restores the blockout's original
visibility after rendering. A first procedural capture omitted the pool. The render-state probe showed
the section was registered, correctly placed and above the landscape; its top-face winding was reversed
for Unreal's renderer. Correcting the winding made the surface visible, and the automation now guards
that index order. A golden-hour capture shows the irregular outline clearly at
`Saved/Viewpoints/2026-10-01_083317_h17.0/02_Tideglass.png`. Matched noon calm/storm captures are in
`Saved/Viewpoints/2026-10-01_083701_h12.0/` and `Saved/Viewpoints/2026-10-01_083749_h12.0/`; the
forced-weather values are restored after each render. The rejected `M_IslandOcean` preview is recorded
in `Saved/Logs/Codex_TideglassWaterPreview_MIslandOcean_20261001.log`; the original baseline and teal
captures remain under `Saved/Viewpoints/2026-10-01_065235_h12.0/` and
`Saved/Viewpoints/2026-10-01_070300_h12.0/`.

UE 5.8.3 validation completed:

- The UE 5.8.3 editor build and `CaptiveSky2.Agent.IslandTideglass` passed. The test checks the
  real saved-map target finder plus synthetic Game-world application, transform/topology/material,
  render-facing winding, no-collision behavior, and restoration/destruction at teardown.
- `CaptiveSky2.Visual.Viewpoints` passed with the same transient geometry at noon and golden hour;
  the rendered image, not the test result alone, confirmed the outline is visible.
- A provider-free Game probe applied the runtime material, completed the Innkeeper's 959 cm route,
  and ended after 11.3 real seconds with zero model requests. It used
  `Saved/Playtests/Codex_TideglassRuntime_20261001` for isolated data.

`-TideglassWeather Calm` or `-TideglassWeather Storm` holds the actual `MPC_IslandEnvironment`
wind/storm/rain scalars at known diagnostic values for the render, then restores their captured
values. This tests the material response, not the weather actor or gameplay integration. Wind is
normalized against a tunable 300 cm/s reference before blending, avoiding immediate saturation at
ordinary breeze speeds. The current procedural sheet is legible and has an uneven shoreline, but the
matched calm/storm frames are visually very similar; make the material's moving swells and weather
response perceptible on this flat geometry before calling the pool art complete. The older flattened-
sphere renders under `Saved/Viewpoints/2026-10-01_073931_h12.0/` and
`Saved/Viewpoints/2026-10-01_074022_h12.0/` show stronger reflection changes, while the new surface
is a better silhouette. The organic surface remains a non-colliding visual prototype without depth,
temperature, water quality, swimming or shoreline blending. Keep the map material reversible and do
not overwrite the authored sphere material.
