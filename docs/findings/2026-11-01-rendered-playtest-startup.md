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

## Symbolization path check — 2026-11-01

The repeated `4DCE2FEABCC3C947BBCB4179F07C8373B60F798D` crash still has only
module-relative offsets in `UnrealEditor-Core` and `UnrealEditor`, so it cannot
yet identify a responsible function. A read-only check of the installed UE
5.8.3 `Engine/Binaries/Win64` found no `UnrealEditor*.pdb` or matching map
files. Visual Studio 2022 Community is installed, but the Windows SDK debugger
folder contains supporting DLLs only and no `cdb.exe` or `windbg.exe`.

Epic's [UE 5.8 installation guide](https://dev.epicgames.com/documentation/unreal-engine/install-unreal-engine)
documents `Editor symbols for debugging` as an optional component that can be
added from the Epic Games Launcher: Unreal Engine > Library > the UE 5.8 tile's
Options > select `Editor symbols for debugging` > Apply. The dialog shows the
additional disk requirement before applying. No component was downloaded or
installed during this check. If the user enables it, verify that the matching
5.8.3 PDBs appear, then open the existing minidump in Visual Studio and resolve
symbols against the exact installed engine binaries before retrying resident
feedback. Until then, do not rerun an equivalent launch; the earlier bounded
15-minute NullRHI resident-feedback session remains the latest successful
character sample.

## Current-worktree feedback launch recheck — 2026-11-02

The guarded `-NullRHICharacterFeedback` launcher was rechecked against both
the worktree's root project and the established `Saved/CS58/CaptiveSky_2.uproject`
scratch project. Both passed the 4 GiB system-memory preflight (10,464 MiB and
10,442 MiB free, respectively), then exited before world-ready at
`SecondsSinceStart=0` with the same null-address `EXCEPTION_ACCESS_VIOLATION`
and `PCallStackHash=4DCE2FEABCC3C947BBCB4179F07C8373B60F798D`. The scratch
launch therefore does not provide a materially different startup path in this
environment, despite being the context used by earlier successful sessions.
Neither attempt reached resident initialization or made model requests; both
used isolated data roots. The latest saved log and dump are under
`Saved/CS58/Saved/Crashes/_0000/` (scratch attempt), while the root-project
attempt's artifacts are under `Saved/Crashes/_0000/`.

Do not repeat either launch unchanged. The most recent successful resident
feedback remains the separately logged 900-second session documented in
[`2026-11-02-resident-feedback-15m.md`](2026-11-02-resident-feedback-15m.md).
Rendered and NullRHI feedback are both blocked pending new crash-symbol or
startup-path evidence; this failure does not invalidate that earlier run.
