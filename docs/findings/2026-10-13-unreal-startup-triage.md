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

## Next useful step

Do not repeat the same launch. The project-opening `-NullRHI` attempt and the
bounded Game launch now have inconclusive early-startup outcomes, one of which
has a concrete null-dereference crash context. A future attempt should first
capture the Unreal PID and exit code independently of the wrapper and preserve
the full command line. Require a project log/world-ready marker or a fresh
crash context as the outcome; do not interpret an exit code without a matching
startup artifact as a pass. Keep agent thinking disabled and the model-request
ceiling at zero. The new crash still needs symbols or a materially changed
startup condition before another live launch is useful.
