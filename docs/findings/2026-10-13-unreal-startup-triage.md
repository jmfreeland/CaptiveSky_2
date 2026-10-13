# Unreal startup crash triage — 2026-10-13

## Evidence checked

- The latest retained crash context reports a null-address access violation at
  `SecondsSinceStart=0` with a Core-only, unsymbolized stack. The project notes
  record both Game and NullRHI project-opening failures; do not repeat an
  unchanged launch.
- This installed UE 5.8.3 tree does not contain `UnrealEditor-Core.pdb`, and
  the standard Windows Kits `cdb.exe` path is absent. The local crash cannot be
  symbolized with the tools currently installed.
- The project explicitly enables the experimental editor-only `AllToolsets`
  aggregator and its individual Toolset plugins. Their engine manifests confirm
  that `AllToolsets` depends on these plugins. `ModelContextProtocol` is a
  separate plugin and was not part of the temporary disable set.

## Reversible isolation attempt

The Toolset entries were temporarily disabled in `CaptiveSky_2.uproject`, then
`UnrealEditor-Cmd.exe` was invoked with `-NullRHI`, `-DisablePython`, zero agent
requests, and an explicit log path. The process returned exit code 0, but it
created neither the requested project log nor a new crash artifact. That is
not evidence of a successful project startup, and it does not confirm or rule
out a Toolset cause. The `.uproject` descriptor was restored; its JSON parses
and all 27 original Toolset entries are enabled again.

## Bounded Game-mode retry — 21:56 local

A controlled close view of the already imported Tidepool Crab was attempted
with the existing spectator launcher: 60 seconds maximum, an isolated data
root, thinking and Python disabled, and `-CaptiveSkyMaxModelRequests=0`. The
launched Game process crashed before the requested project log opened or the
world-ready marker appeared; no screenshots were produced and no Unreal
process remained. The crash context records an unhandled null-address access
violation at `SecondsSinceStart=0` with only an unsymbolized Core/Editor startup
stack (`PCallStackHash 4DCE2FEABCC3C947BBCB4179F07C8373B60F798D`). The captured
launch command line is in `Saved/Crashes/_0000/Codex_TidepoolCrabClose_20261013.log`;
the crash context and minidump are beside it. This confirms that the failure
can happen in Game mode as well as during the earlier Editor/NullRHI attempts,
but it does not identify the null dereference or establish that it is the
source of the separate .NET dialog. Do not repeat the same launch; symbolize
the dump or compare it with the next controlled launch only after the startup
conditions materially change.

## Structured-argument repeat — 22:17 local

A second, isolated Game attempt used a dedicated moonlit MinnowSchool view and
passed arguments through `.NET ProcessStartInfo.ArgumentList` rather than the
spectator script's `Start-Process` argument wrapper. The launched Game PID was
36388. The process exited after about five seconds with exit code 0 before the
Island world-ready marker; it created no requested project log or screenshots.
Crash Reporter did create a fresh startup transcript and minidump. The context
again reports a null-address access violation at `SecondsSinceStart=0` and the
exact same `PCallStackHash` as the 21:56 attempt. This shows the early crash is
not explained solely by the spectator script's argument-array quoting. The
zero exit code is not a pass: the crash context is authoritative. No Unreal
process remained afterward.

## Next useful step

Do not repeat an unchanged launch: the two bounded Game attempts now reproduce
the same zero-second null-dereference stack despite a different launch path and
viewpoint. The installed engine has no `UnrealEditor-Core.pdb`, and no local
WinDbg/CDB or LLVM symbolizer was found, so this machine cannot resolve the
stack with its current tools. Next, obtain matching UE 5.8.3 symbols or inspect
the dump in a symbol-capable debugger; only then choose another controlled
startup experiment. Keep agent thinking disabled and the model-request ceiling
at zero. The repeated startup fault remains separate from the recurring .NET
dialog unless new evidence links them.
