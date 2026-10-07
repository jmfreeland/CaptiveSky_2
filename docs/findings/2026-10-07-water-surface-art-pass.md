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

### Bounded Game retry (2026-10-07)

After a standalone UE 5.8.3 `ValidatePlatforms` invocation completed in 0.21 s,
the same isolated noon Game capture was retried with thinking/Python disabled,
one maximum model request, and a 45-second realtime cap. The attempt used
`Config/TideglassMotionProbe.json` and wrote to
`Playtests/Codex_TideglassWaterArtRetry_20261007/`; no screenshot was produced.

The log reached TurnkeySupport and Windows target-platform registration, then
stopped at `LogSlate` on missing optional VisionOS icon files. It did not record
completion of the in-process `ValidatePlatforms` child, Island loading, or a
window. The editor process remained CPU-active and reported responsive, but
made no log progress for over 90 seconds; it was stopped after approximately
115 seconds. `AutoSDKInfo.txt` remained at its earlier timestamp, so the quick
standalone validator result does not establish that the editor's child process
completed. The stale `dotnet.exe` error dialog/process was not identified or
terminated. Log: `Saved/Logs/Codex_TideglassWaterArtRetry_20261007.log`.

This narrows the retry result: it is not a confirmed deadlock or a rendering
failure, but the target-platform startup path is still not reaching the Island.
`Scripts/Start-Spectator.ps1` now has an opt-in `-StartupTimeoutSeconds` guard.
It waits for the Island's `LogWorld: Bringing World ... up for play` marker,
then leaves the existing play-time/request caps in charge; if startup misses
the timeout, it kills only the launched Unreal process tree and reports the
log path. Ordinary and continuous invocations remain unchanged unless the
parameter is supplied; the option is explicitly rejected for continuous mode.
PowerShell parsing, parameter validation, the readiness-marker match against a
successful Game log line, and `.NET Process.Kill(true)` on a test-owned process
all passed. A follow-up Unreal launch exercised the guard against the real
project: after 90 seconds without the world-ready marker, the script reported
the timeout and its Unreal process tree exited. The only remaining `dotnet.exe`
processes were the two previously observed old husks; no new screenshot was
created and the Island still did not load. Log:
`Saved/Logs/Codex_TideglassWaterArtGuard_20261007.log`.

The guard is now runtime-verified as a safeguard, not as a fix for the
underlying startup problem. The log again stops after TurnkeySupport/target
platform startup; the next step is to inspect that child-process wait rather
than repeat another capture. Do not infer water-art quality from these failed
captures; no rendered comparison exists yet.
