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

## Next useful step

Before another isolation run, verify process creation and log-path handling
independently (capture the executable PID and exit code, and confirm the exact
command line). Then make one controlled, reversible change and require either
a project log/world-ready marker or a fresh crash context as the outcome. Keep
agent thinking disabled and the model-request ceiling at zero. Do not interpret
an exit code without a corresponding startup artifact as a pass.
