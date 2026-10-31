# NullRHI commandlet startup failure

Date: 2026-10-31

## Purpose

Find a safe way to resume bounded resident-feedback runs after the regular NullRHI
launch exited during engine pre-initialization. These tests used the C: scratch
project (`Saved/CS58/CaptiveSky_2.uproject`), not the live D: project, with isolated
data roots, zero model requests, and a 30-second in-game cap.

## Results

1. `UnrealEditor-Cmd.exe` was invoked through `Start-Spectator.ps1` with
   `-UseCommandletExecutable -NullRHI -DisableAgentThinking -DisablePython`.
   The process exited with code 0 before the world's `LogWorld: Bringing World`
   ready marker. The launcher correctly treated that as failure and attached the
   fresh crash artifact rather than counting it as a successful run.
2. The same launch with `-notraceserver` produced the same outcome. Unreal's log
   no longer reported failure to start Unreal Trace Server, but the crash context
   still reported `EXCEPTION_ACCESS_VIOLATION` reading address zero at
   `SecondsSinceStart=0`, with the same PCallStackHash
   `F405610C114D120D4B6F5ABA901EC469AC9FAA26`.
3. The normal `UnrealEditor.exe` launcher, with thinking disabled and no
   commandlet or trace-server override, also exited before world-ready. It
   reproduced `EXCEPTION_ACCESS_VIOLATION` at `SecondsSinceStart=0` with
   `PCallStackHash 4DCE2FEABCC3C947BBCB4179F07C8373B60F798D`, matching the
   earlier 900-second feedback startup failure.

Artifacts are under `Saved/CS58/Saved/Crashes/_0000/`:

- `Codex_CommandletSmoke_20261031.log`
- `Codex_NoTraceServerSmoke_20261031.log`
- `Codex_EditorExeNullRHISmoke_20261031.log`
- `CrashContext.runtime-xml`
- `UEMinidump.dmp`

## Conclusion and next step

The Trace Server error was incidental, not the cause of the commandlet crash.
Both UnrealEditor executables now reproduce an early null-address crash, with
the standard executable matching the prior feedback failure. Do not repeat
these launch variants. Resume feedback only after diagnosing that engine/process
initialization path or after a known-good bounded launch succeeds. The live
editor remained open; no main-project save data was used or modified by these
attempts.

## Elevated NullRHI follow-up (21:45 UTC)

After the main editor was relaunched with access for its normal D: startup
cache/log writes, the same UE 5.8.3 `UnrealEditor-Cmd` binary successfully
initialized the isolated `Saved/CS58` scratch project with NullRHI. A first
`Automation RunTests CaptiveSky2.Agent.IslandWeather` run completed three
Broadleaf child tests, but its filter did not discover the base
`FIslandWeatherTest`. The meadow assertions were moved into the uniquely named
`CaptiveSky2.Agent.IslandWeather.MeadowHabitat` test; after a fresh isolated
build, that exact test passed with exit code 0 and zero model requests. Its log
also reports the commandlet's optional HTTP listener could not bind to
127.0.0.1:8000 while the main editor was open; this did not affect the test.

This establishes a working elevated NullRHI automation path and shows the
earlier startup failures were specific to the prior launch conditions; it does
not yet establish a long-running resident-feedback session. The new meadow test
log is `Saved/CS58/Saved/Logs/Codex_MeadowHabitatAutomation_20261031.log`.
