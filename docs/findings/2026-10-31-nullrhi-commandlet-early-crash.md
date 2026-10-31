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

Artifacts are under `Saved/CS58/Saved/Crashes/_0000/`:

- `Codex_CommandletSmoke_20261031.log`
- `Codex_NoTraceServerSmoke_20261031.log`
- `CrashContext.runtime-xml`
- `UEMinidump.dmp`

## Conclusion and next step

The Trace Server error was incidental, not the cause of this commandlet crash.
Do not repeat the same launch variants. The matching zero-address crash stack
points to a failure before the project world starts. Resume feedback only after
diagnosing that early engine/process initialization path or after a known-good
bounded launch succeeds. The live editor remained open; no main-project save data
was used or modified by these attempts.
