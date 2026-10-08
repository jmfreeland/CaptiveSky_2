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
