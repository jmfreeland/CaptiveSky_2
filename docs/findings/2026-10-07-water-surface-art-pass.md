# Tideglass runtime water readability pass (2026-10-07)

The standalone Game capture showed Tideglass as a bright, nearly uniform cyan
patch. To improve its shallow-water read without editing the generated material
asset or the saved Island map, `UIslandTideglassSubsystem` now creates a
transient `UMaterialInstanceDynamic` and overrides only the pool's existing
parameters at play start. Teardown still restores the authored sphere and
destroys the procedural surface.

The runtime defaults darken and desaturate calm/storm water and edge reflections,
raise calm roughness from the generated candidate's 0.12, and strengthen the
existing calm/weather normals and two animated swells. The generated material
asset remains the instance parent and is not modified. If the optional asset is
absent, the subsystem retains its previous fallback behavior; no Content asset
is written by this change.

## Validation

- UE 5.8.3 `CaptiveSky_2Editor` built and linked successfully (10 actions,
  27.70 seconds).
- `CaptiveSky2.Agent.IslandTideglass` passed under `-NullRHI`. It verifies that
  the live material is a dynamic child of the configured source and that the
  tint, roughness and normal parameters were applied; it also retains the
  existing surface/topology/collision/restoration checks. Log:
  `Saved/Logs/Codex_TideglassWaterArt_20261007_Automation.log`.
- A same-view noon Game screenshot was attempted with an isolated world-state
  root, thinking/Python disabled, one-request maximum, and a 90-second startup
  timeout. Unreal did not reach the Island or create a screenshot: startup
  stopped after loading `TurnkeySupport` while launching UBT's
  `ValidatePlatforms` scan. The editor launched for this capture was stopped;
  a `dotnet` process appeared at the same time as that scan, but its command
  line was inaccessible, so ownership is inferred from timing/log correlation.
  No user editor was open. Log:
  `Saved/Logs/Codex_TideglassWaterArt_20261007_Game.log`.

Therefore this is a compiled and automated, reversible material candidate—not
yet a visually accepted art change. Re-run the same 11:00/12:00 Tideglass view
against the previous capture when the Game/editor launch path progresses past
Turnkey startup. Judge center-water color, edge definition and visible swell
pattern together; revert or retune the transient values if they still read as
an opaque cutout. Do not change the material asset or saved map as part of that
comparison.
