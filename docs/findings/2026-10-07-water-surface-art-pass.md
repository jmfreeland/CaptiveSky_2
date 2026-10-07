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

### Elevated startup comparison (2026-10-07)

The guarded capture was repeated once outside the sandbox, with the same
isolated state, thinking/Python disabled, one-request maximum, 45-second play
cap, and 90-second startup limit. This time the Unreal log did not report the
earlier access-denied deletion of the user `EditorSettings.ini`; nevertheless,
the Game did not produce a window, reach the Island, or write screenshots. The
startup guard timed out and removed the launched process tree. Log:
`Saved/Logs/Codex_TideglassWaterArtElevated_20261007.log`.

Unlike the sandboxed attempts, this run updated `Saved/Logs/AutoSDKInfo.txt`.
That child log ends with `Result: Succeeded` after 0.20 seconds and reports
Win64 valid. Therefore `ValidatePlatforms` completed; it is not the wait causing
the later stall. The Unreal log then stopped during early Slate startup after
optional VisionOS icon lookup warnings, with no matching Application Error or
.NET Runtime event for the launch interval. The launched process tree exited at
the 90-second guard; only the two old `dotnet.exe` husks remained afterward.

The guard is runtime-verified as a safeguard, but these 90-second runs did not
establish an engine-startup hang. The successful Game log
`Saved/Logs/Codex_HighlightGame_20261007.log` contains the same optional
VisionOS Slate icon warnings at 01:13:21, then continues normal startup:
`LoadMap: /Game/Maps/Island` appears at 01:17:41 and the world-ready marker at
01:17:50. That is about 4 minutes 30 seconds to map load and 4 minutes 39
seconds to world readiness, well beyond the failed runs' 90-second limit. The
Elevated log stopping at 90 seconds is therefore consistent with slow startup,
not evidence of a deadlock; the Slate warnings are not a useful stopping
condition.

### Successful bounded retry and visual review (2026-10-07)

A fresh run allowed up to 10 minutes for startup while keeping a 45-second
realtime cap and one-request maximum; agent thinking and Python were disabled,
and all world state and screenshots were isolated under
`Saved/Playtests/Codex_TideglassLongStartup_20261007/`. This run reached the
Island world-ready marker in 37.3 seconds, exited with code 0 under the normal
play cap, and wrote two 1600x900 frames. Log:
`Saved/Logs/Codex_TideglassLongStartup_20261007.log`.

The frame pair confirms the shoreline ecology and moving residents are present,
but the main art goal is still unmet: in both views, the pool reads as a bright,
nearly uniform cyan cutout rather than shallow water, and the surface's wave
structure is difficult to read. The visual change is therefore still an
unaccepted candidate; the next art iteration should test a more visibly
water-like response in the same camera and lighting, with a matched screenshot
comparison before accepting any tuning. Frames:
`Playtests/Codex_TideglassLongStartup_20261007/Screenshots/001_Tideglass.png`
and `002_Tideglass.png`.

### Second transient iteration (2026-10-07)

The matched view led to a second reversible runtime-only adjustment in
`UIslandTideglassSubsystem::TuneReadablePoolMaterial`: calm/storm/edge tints
were darkened, calm/weather roughness raised to 0.34/0.46, calm/weather normal
gain raised to 1.35/1.75, and the long/short swell strengths raised to 0.80/0.48.
The authored material asset and saved map are still untouched. UE 5.8.3 editor
build succeeded (5 actions, then a 4-action test-only rebuild), and
`CaptiveSky2.Agent.IslandTideglass` passed under NullRHI. The automation log is
`Saved/Logs/Codex_TideglassIteration2_20261007_Automation.log`.

The same noon camera then reached world-ready in 15.4 seconds, ran under the
45-second/one-request cap, exited cleanly, and wrote two 1600x900 frames. In the
matched images, the center is darker, broken sky highlights and broad surface
variation are more apparent, and the pool reads more like water than in the
first candidate. This is a visible improvement, though the opaque shallow
blockout still lacks convincing depth and a naturally integrated shoreline;
keep the tuning as a reversible candidate pending a closer art review. Log:
`Saved/Logs/Codex_TideglassIteration2_20261007_Game.log`. Frames:
`Playtests/Codex_TideglassIteration2_20261007/Screenshots/001_Tideglass.png`
and `002_Tideglass.png`.

A separate 17:00 highlight pass used the existing `Config/IslandViewpoints.json`
viewpoints, disabled thinking/Python, and capped play at 120 seconds/one model
request. It reached world-ready in 15 seconds and cleanly produced five frames
before the time cap. `002_Inn_From_Path.png` is the strongest composed view of
the inn, lanterns, meadow edge, and warm evening light; the wide `001` shot
also confirms that large areas of bare terrain remain a landscape-art priority.
Log: `Saved/Logs/Codex_GoldenHourHighlights_20261007.log`.
