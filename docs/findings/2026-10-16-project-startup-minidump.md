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

## Diagnostic limit and next step

The matching `UnrealEditor-Core.pdb` was previously confirmed absent from the
installed UE 5.8 tree, and this crash bundle has no resolved function names.
Do not repeat the same project-opening launch based on this evidence alone.
Install/download the exact matching UE 5.8.3 editor symbols, then resolve the
existing dump with a debugger. Once the Core offsets are symbolized, inspect
the first project-opening caller before choosing any plugin/configuration
change or attempting another bounded test. The saved dump is the authoritative
artifact for that follow-up; the context reports the process started at zero
seconds, and the requested project log was not created.
