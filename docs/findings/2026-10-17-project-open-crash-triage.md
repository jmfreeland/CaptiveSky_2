# Project-open crash triage — 2026-10-17

The Windows `0xe0434352` application errors observed on 2026-10-16 are not, by
themselves, evidence of an Unreal crash. The corresponding `.NET Runtime` 1026
events name `dotnet.exe` / `CaptiveSky.Gateway.Checks.exe` and show unhandled
Gateway test failures: two assertion failures (17:31 and 20:30) and an
`UnauthorizedAccessException` while replacing a headless-turn marker (12:03,
12:04 and 17:21). These should be investigated as Gateway test/runtime issues.

A separate Unreal project-open failure is recorded at
`Saved/Crashes/_0000/CrashContext.runtime-xml`, with matching log
`Saved/Crashes/_0000/CaptiveSky_2.log` and run note
`Saved/Crashes/_0000/Codex_RatHollowTextureAudit_20261016.log`. The command
was a UE 5.8.3 `UnrealEditor-Cmd` project launch using
`-ExecutePythonScript=Scripts/Preview-GeneratedStaticMesh.py`; it crashed at
`SecondsSinceStart=0` with `EXCEPTION_ACCESS_VIOLATION` reading null. The
PCallStackHash is `F405610C114D120D4B6F5ABA901EC469AC9FAA26`. The dump reports
no OOM, and its stack contains only unsymbolized `UnrealEditor-Core` and
`UnrealEditor-Cmd` offsets. The requested `Saved/Logs` file was not created;
the crash-side log ends during engine pre-initialization.

This is distinct from the live editor PID 22688 that the user asked to leave
running; it does not justify terminating or reopening that editor. No build,
project launch, or additional Unreal process was started during this triage.

## Next diagnostic step

Before retrying any project-open automation, locate the exact UE 5.8.3 symbols
or a local minidump debugger and resolve the recorded stack. If symbols are not
available, compare the command-line launch environment and startup plugin/module
loading against a known-good project-open run. Do not infer that the absent
Trace Server, optional profiler DLLs, plant plugin retirement, memory pressure,
or the Gateway exceptions caused this null dereference; current evidence does
not establish any of those causes.
