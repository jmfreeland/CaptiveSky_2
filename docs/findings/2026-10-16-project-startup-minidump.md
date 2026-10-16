# Unreal project-startup crash: minidump module evidence — 2026-10-16

## Evidence

Re-inspected the existing UE 5.8.3 crash bundle in `Saved/Crashes/_0000/`; no
new Unreal process was launched. `CrashContext.runtime-xml` and
`UEMinidump.dmp` record an `EXCEPTION_ACCESS_VIOLATION` reading address zero at
`SecondsSinceStart=0`, with stack hash
`F405610C114D120D4B6F5ABA901EC469AC9FAA26`.

The unsymbolized primary stack begins:

```text
UnrealEditor-Core + 0x5c8346   (faulting instruction)
UnrealEditor-Core + 0x78d875
UnrealEditor-Core + 0x29d733
UnrealEditor-Core + 0x29d3ad
UnrealEditor-Core + 0x5f6930
UnrealEditor-Core + 0x616fa9
UnrealEditor-Core + 0x63bde5
UnrealEditor-Core + 0x31e8f4
UnrealEditor-Cmd  + 0x28437
```

The dump's module list contains 293 modules, all from the engine installation
or Windows; no CaptiveSky project or project-plugin DLL is loaded. This
supports that this particular NullRHI automation launch crashed in the engine
commandlet startup path before project code loaded. It does **not** identify the
root cause or rule out project configuration/data that the engine reads before
loading project modules. It also does not explain the separate `dotnet.exe`
dialogs seen during Unreal build/run.

## Later successful editor session evidence

The separate `Saved/Logs/CaptiveSky_2.log` for 2026-10-16 records a successful
UE 5.8.3 editor session: AutoSDK returned `0` at 09:05, the editor automation
controller discovered project tests, and `CaptiveSky2.Agent.NightEcology`
completed with `Result={Success}` at 10:07. This proves the project can complete
at least one editor automation test in a same-day session. It does not resolve
the zero-address commandlet minidump, prove the editor is currently responsive,
or identify the cause of the separate `dotnet.exe` dialogs. Treat the minidump
and the successful editor session as distinct launches; the observed AutoSDK
exception is not universal.

For the next .NET failure, capture its exact timestamp and correlate that with
Windows Application events and the matching Unreal/BuildTool log before making
an engine, SDK, cache, or project-configuration change. Do not terminate the
current editor solely to repeat a same-day launch that has already succeeded
once.

## Separate .NET WER event — 2026-10-16 17:31 local

Read-only Windows Application log inspection found an `Application Error`
event at 17:31:09 and matching `Windows Error Reporting` record at 17:31:12.
The faulting process was `C:\Program Files\dotnet\dotnet.exe` (host
10.0.1026.32716), running `Microsoft.NETCore.App 8.0.29`; it exited with
exception `0xe0434352` in `KERNELBASE.dll` at offset `0x00000000000c408a`.
The matching `Report.wer` module list includes
`Saved/CompileScratch/Codex_GatewayQuotaBuild/CaptiveSky.Gateway.Checks.dll`
and `CaptiveSky.Gateway.dll`. This identifies a .NET process that loaded the
isolated gateway checks, but the event retained neither the command line nor
the attached temporary dump. It does not identify the managed exception or
prove that the event came from Unreal's build/run path.

A later direct invocation of the lock-holder check exited `0`, and a later full
gateway-check invocation also exited `0` with all checks passing; no new
`dotnet.exe` APPCRASH appeared in the following five-minute event-log window.
The 17:31 record is therefore preserved as an unresolved, non-reproduced
managed exception rather than attributed to either the Unreal startup
minidump or the gateway lock change. For the next popup, record the displayed
time and correlate its process start time/module list and matching UnrealBuildTool
log before making runtime, SDK, or project changes.

## Diagnostic limit and next step

The matching `UnrealEditor-Core.pdb` is absent from the installed UE 5.8 tree,
and this crash bundle has no resolved function names. Epic documents
`Editor symbols for debugging` as an optional engine-install component and says
components can be added later from the Epic Games Launcher without redownloading
the engine ([UE 5.8 install documentation](https://dev.epicgames.com/documentation/unreal-engine/install-unreal-engine)).
When authorized and convenient, open Unreal Engine > Library, choose Options on
the installed UE 5.8.3 tile, select `Editor symbols for debugging`, and Apply.
This component has not been installed from this session.

Do not repeat the same project-opening launch based on this evidence alone.
After the matching symbols are available, resolve the existing dump with a
debugger. Once the Core offsets are symbolized, inspect the first
project-opening caller before choosing any plugin/configuration change or
attempting another bounded test. The saved dump is the authoritative artifact
for that follow-up; the context reports the process started at zero seconds,
and the requested project log was not created.
