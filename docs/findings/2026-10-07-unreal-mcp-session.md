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
