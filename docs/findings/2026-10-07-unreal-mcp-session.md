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

## 2026-10-08 UBT AutoSDK exception-code correlation

A UE 5.8.3 scratch `UnrealEditor-Cmd` run for `CaptiveSky2.Agent.WoodlandDeer`
logged `LogTargetPlatformManager: UBT AutoSDK ReturnCode: -532462766`. Interpreted
as a 32-bit code, `-532462766` is `0xE0434352`, exactly matching the user's
Windows `dotnet.exe - Application Error` dialog. The run continued through map
startup and the automation test passed, so this result identifies the UBT
AutoSDK child path as a source of at least one matching exception status, not
the cause of every dialog or the underlying managed exception. No .NET stack or
matching Windows Application event was available.

The same run reported access denied reading the installed per-user DDC. A
normal scratch `Build.bat` invocation also stalled before producing UBT output;
the same scratch build succeeded after approved elevated access and compiled
only `IslandForestStagTests.cpp` before relinking. This makes restricted access
to UE's per-user cache/log/tooling paths a plausible contributor, but not a
confirmed root cause. The matching-code test log is
[`Codex_RavenStagNotice_20261008_retry.log`](../../Saved/CompileScratch/Codex_RavenCrabAttention_20261008/Saved/Logs/Codex_RavenStagNotice_20261008_retry.log).

Next diagnostic: capture the child command line and managed exception details
for `ValidatePlatforms`/`VerifySdk` while preserving the editor's unsaved
material state. Do not treat the material graph errors or VRAM pressure as the
cause of this CLR code without new evidence.

## 2026-10-09 sandboxed AutoSDK A/B

The same UE 5.8.3 `Build.bat -Mode=ValidatePlatforms -OutputSDKs -AllPlatforms`
command was run against the scratch project twice. Under the normal restricted
workspace token, UE's bundled .NET 10 `dotnet.exe` stayed at 0.14 CPU seconds
for roughly 50 seconds and produced no validation log. Only that diagnostic
process was stopped; the already-open editor (PID 828) remained responsive.
Repeating the command with approved access to UE's per-user support/log paths
completed in about 2.5 seconds, returned exit code 0, and reported Win64 valid
with SDK 10.0.22621.0. The log is
[`Codex_AutoSDK_ValidatePlatforms_20261009_elevated.log`](../../Saved/Logs/Codex_AutoSDK_ValidatePlatforms_20261009_elevated.log).

This controlled result strongly supports restricted access to UBT's per-user
support/log path as a cause of the sandboxed validation stall. Separately, the
bounded scratch Game and automation logs again recorded
`UBT AutoSDK ReturnCode: -532462766` (`0xE0434352`) while automation continued
successfully. The repeated child return code appears in the bounded
[`Game`](../../Saved/Logs/Codex_MinnowSurfaceRing_Game_20261009b.log) and
[`automation`](../../Saved/CompileScratch/Claude_Props/Saved/Logs/Saved/Logs/Codex_MinnowSurfaceRing_Automation_20261009.log)
logs. Together with the earlier `UnauthorizedAccessException` reading
`%LOCALAPPDATA%\UnrealBuildTool`, this ties at least some matching exceptions
to the UE/UBT AutoSDK startup path. It still does not identify the managed
exception stack behind each desktop dialog, and does not prove that every
popup has the same cause; recent Windows Application/.NET Runtime queries
still returned no matching event.

For Codex-launched builds/validation, use approved access to the UE per-user
support paths instead of repeating the known restricted invocation. If a
desktop-editor popup recurs, capture its owning PID and the `ValidatePlatforms`
or `VerifySdk` child stack before restarting Windows. The editor and its
unsaved state were not changed during this A/B check.

## 2026-10-09 RavenPerch and AutoSDK follow-up

A later bounded UE 5.8.3 `UnrealEditor-Cmd` launch against the same scratch
project successfully ran `CaptiveSky2.Agent.RavenPerch`. Resident thinking and
Python were disabled, the request cap was zero, and the real-time play cap was
60 seconds. The test passed; no source, map, or authored Content was changed.
The run's direct `ValidatePlatforms` AutoSDK log completed successfully, but
UE's subsequent startup path still logged `UBT AutoSDK ReturnCode: -532462766`
(`0xE0434352`) and continued into the passing automation. This confirms that
the matching code can recur as a non-fatal UBT AutoSDK startup result; it still
does not reveal the managed exception behind the desktop dialog or prove all
popups share that cause. The corrected run used scratch-local DDC and shader
working directories. An earlier attempt without the shader override failed
separately when UE could not create files in the default shader working
directory, before automation began. Evidence: the [bounded RavenPerch log](../../Saved/CompileScratch/Codex_NestFoundation_20261009/Saved/Logs/Codex_RavenPerch_BoundedRepeat_20261009.log) and its [AutoSDK validation log](../../Saved/CompileScratch/Codex_NestFoundation_20261009/Saved/Logs/AutoSDKInfo.txt).

Next, if the desktop dialog appears again, correlate its timestamp and owning
process with the UBT AutoSDK child while preserving the visible editor state.
The current successful automation is evidence that the code path can be
non-fatal, not a fix for the CLR exception.

## 2026-10-10 repeated popup: live desktop check

The user supplied the same `dotnet.exe - Application Error` dialog again. A
read-only desktop inspection confirmed the modal was still open and showed
exception `0xe0434352` at `0x00007FFD8E6B483A`. The window was exposed under
`csrss.exe`; a contemporaneous process query found no `dotnet.exe`,
`UnrealBuildTool`, `AutomationTool`, `UnrealEditor`, or `UnrealEditor-Cmd`, so
there was no live command line or process tree to correlate with the modal.

The captured desktop image also included a separate-looking Windows alert
naming `UnrealBuildTool` from Epic Games, Inc., with wording about public and
private network access. The alert was not separately targetable in the window
list, and no firewall choice was made. This makes an Unreal
startup attempt plausible, but does not prove that the alert caused this
managed exception or that the popup is from the same process. The latest
bounded saved-Island Game log remains a successful, separate run: AutoSDK
returned 0 and the session shut down normally after 60.1 seconds with zero
model requests. The current exception's managed stack and owner remain
unidentified; the next useful evidence is a fresh process/command line at the
moment of another reproduction, without changing firewall settings blindly.

## 2026-10-10 — Controlled AutoSDK correlation during wildlife automation

While validating the Raven–fox encounter in a scratch Unreal project, a
sandboxed `UnrealEditor-Cmd` startup logged `UBT AutoSDK ReturnCode:
-532462766` (the signed 32-bit form of `0xE0434352`) and
`LogWindows: Warning: CreateProc failed: Access is denied`. Re-running the same
bounded automation with approved access to Unreal's per-user build-log path
returned AutoSDK code `0` and the `RavenPerch` test passed. This strengthens the
correlation between the recurring popup signature and sandbox-denied UBT
platform validation for that launch. It does not identify the managed stack or
prove that every popup comes from this path; the controlled run did not reproduce
the exception once access was granted.

Evidence: [sandboxed diagnostic](../../Saved/Logs/Codex_RavenFoxEncounter_Diagnostics_20261010.log),
[approved run](../../Saved/Logs/Codex_RavenFoxEncounter_Final_20261010.log).

## 2026-10-10 standalone AutoSDK check after another popup

The user supplied the same `0xe0434352` dialog again, now showing instruction
address `0x00007FF841A2483A`. A fresh read-only Application-log query found no
matching `dotnet.exe` or .NET Runtime crash event, and no `dotnet.exe`, UBT,
AutomationTool, or Unreal Editor process was running to correlate with it. In a
separate approved-access check, the installed UE 5.8.3 `Build.bat
-Mode=ValidatePlatforms -OutputSDKs -AllPlatforms` completed successfully and
reported Win64 SDK 10.0.22621.0 valid; the other platforms reported invalid
because their SDKs are not installed/configured. This confirms that the
standalone AutoSDK validation path works with approved access now, but it did
not reproduce the desktop dialog or the editor's full `VerifySdk` launch. The
managed exception behind this particular popup remains unknown.
