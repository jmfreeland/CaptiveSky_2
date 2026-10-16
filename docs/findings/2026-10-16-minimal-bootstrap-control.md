# UE 5.8 Empty-Project Startup Control

## Result

An empty, isolated UE 5.8 control project completed editor initialization without reproducing the known CaptiveSky startup crash.

The control project was `Saved/CompileScratch/Codex_UE58_MinimalBootstrap_20261016/MinimalBootstrap.uproject`. It had no game modules, content, or project plugins enabled. It ran with `-NullRHI`, unattended, a project-local Derived Data Cache, and `-ExecCmds=Quit`. The log reached `LogLoad: (Engine Initialization) Total time: 48.82 seconds`, recorded the `Quit` command, completed its Asset Registry scan, and wrote its project-local cache. The process was no longer present at follow-up inspection. No fatal-error or unhandled-exception marker was found in the log.

The process exit code was not retained in the available tool output, so this establishes that the empty control reached normal editor initialization and shut down, not that every shutdown step was clean.

## Interpretation

This makes a universal UE 5.8 engine/bootstrap failure less likely. It points the next investigation toward differences introduced by CaptiveSky's project configuration, project plugins, modules, or content. It does not identify which of those is responsible: the control did not load the CaptiveSky project or its plugins, and it used `-NullRHI` rather than the normal renderer.

The CaptiveSky project should not be relaunched repeatedly with the same startup setup. A useful next diagnostic is a controlled project-copy bisect that disables groups of project plugins and then restores them by halves, without editing the user's main `.uproject` or project content. If a project copy still crashes, preserve its crash report and minidump for symbolized analysis before trying more runtime variants.

## Scratch artifacts

The disposable control project and its generated logs/cache remain under `Saved/CompileScratch/Codex_UE58_MinimalBootstrap_20261016/` for reference. They are local diagnostics, not project assets.
