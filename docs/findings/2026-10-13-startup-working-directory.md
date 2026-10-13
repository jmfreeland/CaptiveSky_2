# Unreal startup working-directory finding (2026-10-13)

## Evidence

The latest bounded spectator launch (`Saved/Crashes/_0000/`, 2026-10-13
13:12:40) terminated with a null-address access violation in UE 5.8.3. Its
unsymbolized game-thread stack contains only `UnrealEditor-Core` and
`UnrealEditor` frames; the crash context has no source-level call stack or
register dump. The matching `UnrealEditor-Core.pdb` and `UnrealEditor.pdb` are
not installed beside the engine binaries, and this machine has no `cdb`,
`windbg`, or `dumpchk` command available. This does not identify the null access
as an engine defect or rule out project/plugin startup code.

The same startup log reports that it could not launch
`../../../Engine/Binaries/Win64/UnrealTraceServer.exe`. The executable is
present at `D:\Games\Epic\UE_5.8\Engine\Binaries\Win64\UnrealTraceServer.exe`.
`Scripts/Start-Spectator.ps1` previously inherited the caller's working
directory for all launch paths; from the project root, that engine-relative
helper path cannot resolve. The failed trace-server launch falls back to an
in-memory trace and is not, by itself, evidence for the null crash.

## Change and verification

The launcher now uses the installed engine's `Engine/Binaries/Win64` directory
as its working directory for direct, bounded, and continuous launches. This
preserves the script's project-root resolution of project, screenshot, DDC,
shader, and data paths while allowing engine-relative helpers to resolve.

Validation: PowerShell parses the edited script without syntax errors; the
engine-relative UnrealTraceServer path resolves from the configured working
directory and exists. No Unreal process was started, and no startup success is
claimed.

## Next step

Install the matching UE 5.8.3 **Editor Symbols for Debugging** component through
Epic Games Launcher ([Epic's UE 5.8 documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/context-switches-in-unreal-engine-5)
identifies this Launcher option), then symbolicate the existing minidump before
retrying the project. Reassess the call stack and startup log after the
trace-server warning is gone. If the crash still reproduces, use the newly
symbolized frames to isolate engine, plugin, or project initialization. Keep
the startup/retry gate in place until that evidence is available.
