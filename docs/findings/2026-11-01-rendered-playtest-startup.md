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

## Cross-mode startup recheck — 14:37–14:40 UTC

A fresh 15-minute rendered resident-feedback launch used the current
`Saved/CC/Project` scratch project, a 40-request ceiling, isolated playtest
data, and both 4 GiB preflights plus live GPU/RAM monitors. Preflight passed at
5,537 MiB free GPU and 11,678 MiB free physical RAM. The Game exited before
world-ready with exit code 0; its crash context records a null-address
`EXCEPTION_ACCESS_VIOLATION`, `SecondsSinceStart=0`, and the same
`4DCE2FEABCC3C947BBCB4179F07C8373B60F798D` fingerprint. No character request
or playtest state was produced.

A second, 30-second zero-request startup diagnostic used
`-DDC-ForceMemoryCache -ddc=NoZenLocalFallback` to bypass the Zen-backed DDC
path. Its preflight passed at 5,537 MiB GPU and 11,577 MiB physical RAM free,
but it exited with the identical zero-second crash fingerprint. This weighs
against Zen/DDC being the cause of this access violation; the separate Zen
slow-start warning remains a recovered cold-start/contention issue. The crash
log also reports Unreal Trace Server error `0x0000a005`, but does not establish
that as causal. The minidump still has only module offsets, and installed UE
symbols/debugger remain unavailable. Do not retry the same Game startup with
or without DDC bypass; continue source-side work until the dump can be
symbolized or a genuinely different startup path is validated.

The guarded launcher terminated both attempts within its own scope. After the
second exit, no Unreal or CrashReportClient process remained; a follow-up
sample showed 5,253 MiB free GPU and 11,699 MiB free physical RAM. The earlier
bounded 15-minute NullRHI feedback run is still the latest successful
character-feedback sample.

Artifacts:

- Cross-mode crash context: `Saved/CC/Project/Saved/Crashes/_0000/CrashContext.runtime-xml`
- Rendered crash log: `Saved/CC/Project/Saved/Crashes/_0000/Codex_ResidentFeedback_20261101_143340.log`
- DDC-bypass crash log: `Saved/CC/Project/Saved/Crashes/_0000/Codex_StartupMemoryDDC_20261101_1440.log`
- DDC-bypass Unreal log: `Saved/CC/Project/Saved/Logs/Codex_StartupMemoryDDC_20261101_1440.log`
