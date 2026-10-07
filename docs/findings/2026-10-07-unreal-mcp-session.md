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
