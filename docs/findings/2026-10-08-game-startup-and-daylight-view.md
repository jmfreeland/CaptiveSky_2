# Bounded Game startup and Tideglass view (2026-10-08)

This recheck did not reboot Windows. It used the existing UE 5.8.3 Game path
with agent thinking and Python disabled, isolated playtest data roots, explicit
real-time/request caps, and no changes to the saved Island map or Content assets.

## Startup findings

- A normal sandboxed Game launch failed during Derived Data Cache setup:
  `Unable to use cache graph 'Installed' because it has no writable nodes
  available`. The engine log recommends `-DDC-ForceMemoryCache` for this
  permission-limited case.
- Retrying with `-DDC-ForceMemoryCache -ddc=NoZenLocalFallback` passed that
  failure but stalled in Turnkey `ValidatePlatforms` and hit the script's
  180-second startup timeout. The script terminated only the Game process tree
  it had launched.
- The same bounded launch with elevated access reached the Island. Its first
  warm startup took 62.7 seconds; a subsequent launch reached world-ready in
  21.2 seconds. This is consistent with the sandbox being unable to access UE's
  per-user DDC/Turnkey support files, not evidence that a Windows reboot is
  required for the standalone Game path.
- A 90-second golden-hour run captured four frames and ended cleanly. A second
  120-second run captured the Tideglass view at 17:58, by which time the scene
  was dark. A final 40-second daylight run captured two Tideglass frames at
  11:00 and 11:24. All runs used isolated `Saved/Playtests/` roots, disabled
  agent thinking and Python, and ended with zero model requests.
- An elevated Editor launch with the same DDC workarounds completed Turnkey's
  SDK detection (`ExitCode=0`) and loaded many engine modules, but did not
  expose a usable Editor window or MCP endpoint within three minutes. The only
  visible Unreal window was its log console; a `dotnet.exe - Application
  Error` dialog showed exception `0xe0434352`. I stopped only the Editor
  process tree launched for this probe after the timeout.

After the Editor probe, no Unreal Editor process remained and `127.0.0.1:8000`
was still closed. The two old parentless `dotnet.exe` entries (PIDs 40100 and
52284) also remained, yet the elevated Game runs completed without them being
cleared. A reboot may still be a convenient way to clear those process husks,
but it is not required to run the bounded Game workflow. The Editor/MCP startup
failure remains unresolved; the exception code alone does not identify which
.NET component failed.

## Visual evidence

The daytime standalone Game frame is
[`001_Tideglass.png`](../../Saved/Playtests/Codex_ContinuationDaylightTideglass_20261008/Screenshots/001_Tideglass.png).
It shows a convincing reflective pool and visibly active residents, with
dragonflies overhead, but the composition remains uneven: crowded foreground
plants, a broad exposed brown middle distance, and sparse/thin tree silhouettes.
The Tideglass surface and its immediate habitat read more clearly than in the
previous editor-world preview; the wider ecological sequence still does not.

The Game log continues to warn that 13 Rhodo Ever material instances lack the
`InstancedStaticMeshes` usage flag and fall back to Default Material. The frame
contains pale default-looking foliage, so imported-material repair remains a
prerequisite to judging those flowers or expanding their use. No binary
`Content/` assets were changed in this run.

The earlier golden-hour Shore/Inn frames are under
[`Codex_ContinuationHighlight_20261008_Elevated/Screenshots`](../../Saved/Playtests/Codex_ContinuationHighlight_20261008_Elevated/Screenshots/).
They are useful startup/presentation checks, not proof of resident agency or a
30-FPS traversal result. The 17:58 Tideglass frame is intentionally not treated
as a daylight composition comparison.

## Next steps

1. To validate the Editor/MCP route, start the GUI in the user's interactive
   session with access to the UE per-user support files; then check the editor
   process and MCP port separately. The bounded Game result alone does not
   establish either.
2. Coordinate with the current foliage owner before editing `IslandWeather.*`.
3. Coordinate/authorize any binary `Content/` material repair, then rerun the
   same 11:00 Game camera and a separate bounded PIE traversal profile. Keep the
   screenshots and performance measures distinct.

## GUI startup recheck (2026-10-08)

A fresh desktop inspection still finds no Unreal Editor window or process. The
Windows desktop has a `dotnet.exe - Application Error` dialog reporting
`0xe0434352`, while `127.0.0.1:8000` remains closed. Recent Application-log
queries did not expose a matching .NET Runtime or Unreal crash event, so the
throwing component is still unidentified. The UE 5.8.3 headless
`CaptiveSky2.Agent.RavenPerch` test passed in the same session, confirming that
the project module can load and exercise the provider-free behavior; this does
not validate the graphical Editor startup path. A system reboot has not been
performed. If authorized, reboot once, then make one bounded Editor launch and
check the process, visible window, and MCP port independently.

## Live editor recheck (2026-10-08, later)

The editor is now reachable through Unreal MCP: `CaptureEditorImage`,
`GetCameraTransform`, `GetSelectedActors`, and `IsPIERunning` responded. The
Island level is open, no actor is selected, and PIE is not running. The editor
tab/status bar shows `Island*` and `1 Unsaved`, so the viewport and level were
left untouched. The captured editor image showed no `dotnet.exe` error dialog;
this does not identify or resolve the earlier exception. The older parentless
`dotnet.exe` entries remain an unverified separate observation. The current
editor/MCP check did not require or initiate a reboot, though the recurring
managed exception still requires a Reliability Monitor or equivalent
faulting-application record to identify its source.

## Tideglass ecology-cue visual recheck (2026-10-08)

A separate main-project Game process reached the saved Island with the existing
UE 5.8.3 runtime, using `-CaptiveSkyDisableAgentThinking`, `-DisablePython`, an
isolated `-CaptiveSkyDataRoot`, a 90-real-second cap, and a one-request ceiling.
It ended normally after 90.1 seconds with zero model requests. The open editor
and its unsaved map were not used or saved. A prior attempt against a fresh
scratch project hit its 180-second startup ceiling during cold shader/cache
initialization; it never reached the map. Using the main project's already
available shader/DDC path reached world-ready in 71.3 seconds. Logs:
[`Codex_TideglassEcologyCue_MainProject_20261008.log`](../../Saved/Logs/Codex_TideglassEcologyCue_MainProject_20261008.log)
and [`Codex_TideglassEcologyCue_Elevated_20261008.log`](../../Saved/Logs/Codex_TideglassEcologyCue_Elevated_20261008.log).

The 1600×900 Game sequence produced three frames at Island-clock 09:00, 09:14,
and 09:29, despite the startup request for hour 11; the requested hour override
did not take effect and must be fixed before a matched 11:00 comparison:
[`001_Tideglass.png`](../../Saved/EcoCue_MainProject_20261008/Screenshots/001_Tideglass.png),
[`002_Tideglass.png`](../../Saved/EcoCue_MainProject_20261008/Screenshots/002_Tideglass.png),
and [`003_Tideglass.png`](../../Saved/EcoCue_MainProject_20261008/Screenshots/003_Tideglass.png).
They show the reflective pool, the transient stone presentation, a moving stag,
and dragonflies, so the wider habitat feels more inhabited than a static
landmark shot. They do not demonstrate the Raven/minnow behavior specifically.
The same frame still has a crowded near edge, open brown middle-distance slope,
thin distant tree silhouettes, and the Inn cut off at frame-left. The runtime
again reports 13 Rhododendron Ever material instances missing the
`InstancedStaticMeshes` usage flag and falling back to Default Material. Repair
of those ignored `Content/` assets is pending explicit user coordination; no
Content asset was changed. This is a composition/readability check, not a
30-FPS gameplay profile or validation of voluntary agent behavior.

Next: once the imported-flower repair is authorized and the hour override is
reliable, rerun a matched 11:00 camera after restoring the flower readability,
then do a separate PIE traversal profile with the foliage owner's current
`IslandWeather.*` changes. Keep the 30-FPS p95 gate and don't infer performance
from these frames.
