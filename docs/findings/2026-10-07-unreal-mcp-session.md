# Unreal MCP is configured but not currently running (2026-10-07)

The project configuration still points both `.codex/config.toml` and `.mcp.json`
to `http://127.0.0.1:8000/mcp`. In this session, Windows exposed the Epic Games
Launcher but no Unreal Editor window; a process check found no `UnrealEditor`,
`UnrealEditor-Cmd`, or `UnrealBuildTool`. A TCP listener check also found no
process bound to port 8000, and the current Codex tool inventory contains no
Unreal MCP tools.

This separates the symptoms: the project has an MCP endpoint configured, but
the editor-side service is not currently serving it, so a machine reboot is
not the first diagnostic step. Reopen the CaptiveSky project in UE 5.8.3 and
allow startup to finish; then confirm the editor process and listener on port
8000. If the listener is present but this Codex session still has no Unreal
tools, reload/restart the Codex session so it re-reads the project MCP config,
then verify the endpoint handshake before running automation. Do not infer a
permanent MCP failure from startup delay alone: the 2026-10-01 live validation
found the same endpoint became available after editor initialization.

No Unreal project, level, asset, or editor setting was changed during this
diagnostic. The current Island work still needs bounded runtime validation
once UE and the MCP endpoint are available.

## 2026-10-08 recheck

The configured endpoint still fails its handshake: `list_toolsets` returns a
transport error while requesting `http://127.0.0.1:8000/mcp`. A process query
did not list `UnrealEditor` or `UnrealEditor-Cmd`, and `netstat -ano` showed no
listener on port 8000. A CIM process query was denied access, so that query is
not treated as evidence. The currently useful next step remains reopening the
CaptiveSky project and checking the editor-side server, not rebooting Windows.
If the endpoint appears after the editor finishes loading but this session
still cannot connect, restart/reload the Codex session before considering a
machine reboot. No editor or server process was started, stopped, or modified
by this check.

## 2026-10-08 project-launch reproduction

The project was then launched once with the installed UE 5.8.3 editor. Startup
initialized D3D12 on the RTX 4080 Laptop GPU and reached `TurnkeySupport`,
which began serialized `VerifySdk` through `RunUAT.bat`. Before the editor
window or MCP listener became available, Windows displayed a
`dotnet.exe - Application Error` dialog with exception `0xe0434352`, matching
the user's screenshot. The editor process remained alive with a blank window
title, while TCP port 8000 remained closed. The project log stops immediately
after the Turnkey command launch; no `Intermediate/TurnkeyLog_0.log` was
created. A recent Application-log query returned no matching records, so the
specific .NET exception source is not established yet.

This reproduces a startup failure in the SDK-verification path; it does not
show that Windows itself needs a reboot. Keep the visible exception available
for inspection and capture the failing Turnkey/.NET child details before
restarting the machine. No level, asset, or source file was changed by the
launch.

## Process-chain follow-up

The launch's two active Unreal-bundled .NET commands were identified: UBT
`ValidatePlatforms` and AutomationTool `Turnkey ... -WaitForUATMutex -command=VerifySdk`.
No other `UnrealEditor`, `UnrealBuildTool`, or `AutomationTool` process was
present. Their output files did not advance beyond the existing startup log.
The machine also retains two older `dotnet.exe` entries created on 2026-09-29
and 2026-10-01; their recorded parent processes are absent and their executable
paths are unavailable. Those entries predate this editor launch. The visible
error window's owning PID could not be identified, and the Application and
.NET Runtime event-log queries returned no matching records, so a causal link
between the old entries, the dialog, and Turnkey's wait is not proven.

A clean Windows reboot is now a reasonable diagnostic experiment if user work
is saved: it should clear the parentless entries and any stale dialogs, but it
is not a confirmed fix for the SDK-verification failure. After restart, verify
the old entries are gone, then launch the editor once and capture fresh
Turnkey/.NET logs if the failure returns. Avoid killing the opaque old PIDs
individually; their ownership and executable paths remain unavailable.

## Startup-process cleanup

The only editor instance in the process list was the one started for this
diagnostic (PID 12728); it remained at the same Turnkey log line with a blank
title. Its process tree was stopped. The active `ValidatePlatforms` and
`VerifySdk` parents terminated; two short-lived child PIDs returned access
denied during tree termination, then disappeared from the subsequent process
inventory. The Windows `dotnet.exe - Application Error` dialog remained
visible, but its owning PID was not identified. The two older parentless .NET
entries (created 2026-09-29 and 2026-10-01) remain, and port 8000 is still
closed. No other editor was stopped, and no project content was loaded or
changed. Do not retry editor launch before the stale-process state is cleared.

## 2026-10-08 MCP recovery

The configured Unreal MCP endpoint became usable after launching UE 5.8.3 as a
hidden editor process (PID 828). `list_toolsets` returned the registered editor,
automation, Niagara, PCG, and other toolsets; editor queries confirmed
`/Game/Maps/Island` is loaded and returned visible actors and a viewport image.
This supersedes the earlier recommendation to reboot as the next step: a full
machine reboot was not needed to restore MCP access.

This is not confirmation that the user's visible desktop editor is connected.
The recovered process has no main-window title and uses roughly 8.1 GB of
working memory. Its viewport camera was aimed at a coastal slope, not the
Tideglass composition. Keep this process rather than launching a duplicate
while MCP access is needed; if a visible editor window is required, reopen the
project visibly only after checking that PID 828 has exited or deliberately
closing that isolated process. The earlier bounded Simulate session was stopped
and `IsPIERunning` returned false. The recovery launch used a separate
world-data root, disabled resident thinking, and set the model-request cap to
zero; no world-state writes or resident requests were intended. The editor log
is [`Codex_EditorMcpRecovery_20261008.log`](../../Saved/Logs/Codex_EditorMcpRecovery_20261008.log).

## 2026-10-08 reachability recheck

`CaptureEditorImage` still returns the Island editor UI, and `IsPIERunning`
returns false after the short Simulate session was stopped. A Windows process
query shows PID 828 responsive, but with an empty `MainWindowTitle`; no listener
is present on local port 8000. This proves the Unreal MCP tool route can reach
an editor image, but it does not prove that this is the visible desktop editor
the user opened. The captured Island shows `1 Unsaved`; do not close, restart,
or save that level without an explicit choice about its unknown edit.

No reboot is indicated by this evidence: the reachable process is responsive,
and the mismatch is specifically between MCP image access and the visible
desktop/process endpoint. If direct control of the user's visible editor is
needed, identify the window/process pairing first, while preserving its
unsaved level.
