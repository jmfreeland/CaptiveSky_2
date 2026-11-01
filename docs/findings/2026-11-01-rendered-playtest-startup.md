# Guarded rendered playtest startup — 2026-11-01

## Memory preflight and monitor

The supervised rendered-play path originally read available physical memory
through `Get-CimInstance Win32_OperatingSystem`. This desktop session denies
CIM access, and `Get-Counter '\\Memory\\Available MBytes'` also fails here.
That meant the launcher's optional 4 GiB system-memory preflight and monitor
could not run, even though Task Manager showed memory headroom.

`Scripts/SystemMemoryBudget.psm1` now queries the Windows
`GlobalMemoryStatusEx` API directly. Both the launch preflight and the periodic
monitor use the same MiB-returning helper; the existing fail-closed threshold
and two-consecutive-low-samples stop rule are unchanged. The focused PowerShell
test passed, including a live positive physical-memory reading. At the rendered
attempt below, the launcher measured 14,045 MiB available and required 4,096 MiB.

## Rendered attempt

The isolated `Saved/CS58` project was launched through `Start-Spectator.ps1` in
1280x720 windowed UE 5.8.3 Game mode, using the rebuilt worktree module and
read-only link to the main project's content. The launch requested scenic
viewpoint shots, situation logging, Python disabled, a fresh isolated playtest
data root, a 900-second session cap, and a 40-model-request ceiling. Both GPU
and physical-memory 4 GiB preflights and 5-second live monitors were enabled.

The GPU preflight passed at 6,739 / 12,282 MiB free; the physical-memory
preflight passed at 14,045 MiB free. The child then crashed before the Island
world-ready marker, at `SecondsSinceStart=0`, with
`EXCEPTION_ACCESS_VIOLATION` reading `0x0000000000000000`. Its hash
`4DCE2FEABCC3C947BBCB4179F07C8373B60F798D` matches the earlier unsymbolized
startup failures. No feedback session, model request, or viewpoint screenshot
was produced. After exit, free VRAM was 6,455 MiB and available physical memory
was 13,970 MiB, so the crash was not a memory-threshold shutdown.

The dump's stack contains only `UnrealEditor-Core` and `UnrealEditor` module
offsets. The expected `UnrealEditor`, `UnrealEditor-Core`, `UnrealEditor-Engine`,
and project PDBs are not present in the installed UE 5.8.3
`Engine/Binaries/Win64`, and `windbg.exe`/`cdb.exe` are not available on PATH.
Do not repeat this equivalent standalone Game launch unchanged. The rendered
woodland, player shoreline traversal, frame-time, and A/B silhouette checks
remain open until the crash is symbolized or a materially different startup
path is validated. The earlier bounded 15-minute NullRHI resident-feedback run
remains the latest successful character-feedback sample.

Artifacts:

- Crash context: `Saved/CS58/Saved/Crashes/_0000/CrashContext.runtime-xml`
- Crash log: `Saved/CS58/Saved/Crashes/_0000/Codex_RenderedResidentFeedback_AB_20261101_0853.log`
- Minidump: `Saved/CS58/Saved/Crashes/_0000/UEMinidump.dmp`
