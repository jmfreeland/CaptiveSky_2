# UnrealBuildTool .NET startup exception (2026-10-08)

## Finding

The dialog reports `dotnet.exe - Application Error` with exception
`0xe0434352`. During an isolated UE 5.8.3 Game launch, the engine logged:

```text
LogTargetPlatformManager: UBT AutoSDK ReturnCode: -532462766
```

`-532462766` is the signed 32-bit representation of `0xe0434352`. The
matching return code identifies UnrealBuildTool's AutoSDK/platform-validation
child process as one concrete source of this exception type in the tested
launch. The engine continued loading, initialized the Island, and completed a
bounded resident movement probe; this was not a gameplay-process crash. The
matching code makes UBT a plausible source for dialogs with this signature, but
does not identify the process behind the pictured live occurrence.

The editor/game launches the bundled UE .NET 10 host for a command equivalent
to:

```text
Build.bat -Mode=ValidatePlatforms -OutputSDKs -AllPlatforms -project=<scratch project> -log=<scratch Saved/Logs/AutoSDKInfo.txt> -verbose -timestamps
```

Windows' Application log had no matching recent `.NET Runtime`/`Application
Error` record with a stack trace. The standard
`C:\Users\freel\AppData\Local\UnrealBuildTool` directory is inaccessible from
this restricted execution context (`Access denied`). A direct reproduction
with an explicit workspace log path also remained blocked before creating that
log and was interrupted. This matches the earlier coordination note that UBT
cannot access its standard local log path in the restricted context, while an
approved elevated UBT run had previously succeeded. The old scratch
`AutoSDKInfo.txt` contains a successful validation from an earlier timestamp;
it is stale and is not evidence that this occurrence succeeded.

## Live-editor cross-check

On 2026-10-08, the connected user's Unreal Editor (PID 828) was still running.
Its own session log showed the normal `ValidatePlatforms` invocation and then
`LogTargetPlatformManager: UBT AutoSDK ReturnCode: 0` at `02:57:34`. Thus this
editor session's startup validation completed successfully; the screenshot's
dialog is not explained by a failing AutoSDK validation in that session. A
read-only process check also found two tiny `dotnet.exe` entries (PIDs 40100 and
52284) with no reported executable path, start time, or main-window handle.
The desktop inspection surface did not expose any app windows, and the Windows
Application log still had no matching managed stack. We therefore cannot
reliably assign the displayed dialog to either process or to this editor.

This narrows the next diagnostic: capture the process ID and fresh UBT log at
the instant the dialog appears, rather than treating the current editor's
successful validation as the cause. The inaccessible standard UBT log directory
remains a limitation for the restricted shell context; don't change ACLs or
disable validation to work around it.

## Repeat occurrence check

After the same dialog was shown again on 2026-10-08, a read-only check found no
`dotnet.exe` process at the time of inspection and no matching `.NET Runtime`,
`Application Error`, or Windows Error Reporting event naming `dotnet.exe` or
`0xe0434352` in the preceding two days. This still does not identify the process
behind the pictured occurrence. The dialog's generic CLR exception code is not
enough to assign blame to UnrealBuildTool; capture the process and timestamp
while it is present, then correlate those with a fresh event/log entry.

### Latest recurrence while a bounded PIE session was running

During a later report of the same dialog, two tiny `dotnet.exe` process entries
(PIDs 40100 and 52284) were visible, but process command lines and owner details
were inaccessible. Windows' Application log had no matching recent event, and
no recent Windows Error Reporting `Report.wer` named `dotnet.exe` or the CLR
exception code. An offscreen UE Editor process (PID 5584) was running Claude's
bounded `Claude_Props` dew PIE capture at the same time. Its log records
`UBT AutoSDK ReturnCode: 0` during startup and a normal `LogExit: Exiting`; it
contains no `0xe0434352`, unhandled-exception, or fatal-error entry. It does
contain an unrelated material warning: `/Game/Materials/M_IslandDew` lacks the
`InstancedStaticMeshes` usage flag and falls back to Default Material in Game.

This rules out a failing AutoSDK validation in that particular UE process and
shows that its gameplay session completed normally. It does not identify the
source of the dialog: the two `dotnet.exe` entries still cannot be tied to a
parent or command line, and absence of a Windows event is not proof that no
other managed process failed. Keep the next diagnostic unchanged: while one
dialog is visible, capture its timestamp and the owning process ID/command line
in Task Manager or Process Explorer, then correlate with the corresponding
fresh log or event. Do not alter SDK validation or Windows permissions.

### Timestamped UBT correlation (2026-10-08, 18:02–18:04 local)

During the next visible dialog, the elevated process snapshot found a new live
`dotnet.exe` (PID 55928) running the UE 5.8.3 bundled .NET 10 host and
`UnrealBuildTool.dll` for `Saved/CompileScratch/Claude_Props/CaptiveSky_2.uproject`:

```text
CaptiveSky_2Editor Win64 Development -WaitMutex -MaxParallelActions=3 -NoHotReloadFromIDE
```

Its start time was 18:02:08. The matching `dotnet.exe - Application Error`
window was present at the same time, but Windows exposed that dialog under
`csrss.exe`; no window-to-process ID was available. The UBT process had exited
by 18:04:42 while the dialog remained. No matching `.NET Runtime`, Application
Error, or WER event supplied a managed exception type or stack trace. This is a
strong process/time correlation to Claude's scratch build, not proof that PID
55928 owned the displayed dialog or the root cause of its exception.

After that process exited, a separate elevated UE 5.8.3 scratch build for
`Codex_RavenCrabAttention_20261008` used the same bundled .NET host with
`-NoHotReloadFromIDE` and `-MaxParallelActions=2`; it compiled successfully.
The focused `CaptiveSky2.Agent.RavenPerch` automation then passed with agent
thinking disabled and model requests capped at zero. This shows that the
toolchain can complete a controlled build/test in the same environment; it
does not explain why the earlier Claude build surfaced the dialog. The dialog
was no longer visible in the later Unreal Editor capture. Keep the cause open
until a recurrence is tied to its exact process and fresh UBT output.

### Repeatable ValidatePlatforms exception code (2026-10-08, 19:31–19:34 local)

A fresh isolated UE 5.8.3 headless launch for the Raven attention regression
again ran the bundled .NET 10 host with
`Build.bat -Mode=ValidatePlatforms`. Its UE log recorded
`LogTargetPlatformManager: UBT AutoSDK ReturnCode: -532462766` on two
consecutive launches. In both runs, the separate `AutoSDKInfo.txt` reported
`Result: Succeeded` and a 0.21-second execution; the last process loaded the
scratch `CaptiveSky_2` module, passed `CaptiveSky2.Agent.RavenPerch`, and exited
with code 0. The editor's own in-session validation continued to report
`ReturnCode: 0`.

This confirms that the matching CLR exception code is repeatedly returned by
the Turnkey/UBT validation child during otherwise successful headless editor
startup. It makes that child a strong candidate source for the recurring
`dotnet.exe` popup, but the dialog's owning process was not captured in this
turn and no managed exception type or stack trace was obtained. Do not infer
that Windows/.NET installation repair, SDK deletion, or disabling platform
validation is justified by the generic code alone.

The final verification log is
[`Codex_RavenRainBasinAttention_verify.log`](../../Saved/CompileScratch/Codex_RavenCrabAttention_20261008/Saved/Logs/Codex_RavenRainBasinAttention_verify.log);
its adjacent `AutoSDKInfo.txt` is scratch output. The build and test used the
isolated scratch project's binaries and data root; the open editor and its
unsaved material were not altered.

### Bounded Game startup correlation (2026-10-09, 02:36 local)

A new isolated UE 5.8.3 Game launch started as PID 41764 at 02:36:31. Its log
records `Launching UnrealBuildTool... -Mode=ValidatePlatforms` at 02:36:35;
at that same time, a new `dotnet.exe` (PID 43908) appeared from UE's bundled
.NET 10 directory. The process remained at 0.16 CPU seconds while the Game log
stopped at `TurnkeySupport` startup. The live `AutoSDKInfo.txt` was unchanged
from the preceding successful run at 02:33 and still said `Result: Succeeded`,
so it is not evidence that this new validation completed.

The desktop still showed the generic `0xe0434352` dialog during this run. The
window was exposed under `csrss.exe`, so there is still no direct window-to-PID
link or managed exception stack. The simultaneous bundled host, validation
launch, idle child, and stalled Game startup make this the strongest correlation
so far, but do not prove that PID 43908 owned the dialog or reveal the underlying
exception. After the log remained unchanged for over a minute, only this run's
Game process and newly spawned .NET process were stopped; the interactive editor
(PID 828) was left running. The previous bounded run and latest automation both
completed with `AutoSDK ReturnCode: 0`; this intermittent failure is not a
reproducible failure on every launch. No matching recent `.NET Runtime` or
`Application Error` event supplied a stack.

Evidence: [`Codex_MinnowEyes2_Game.log`](../../Saved/Playtests/Codex_MinnowEyes2_20261009/Codex_MinnowEyes2_Game.log)
and the 02:33 `Codex_MinnowEyes2_Automation.log`
([compile scratch log](../../Saved/CompileScratch/Claude_Props/Saved/Logs/Codex_MinnowEyes2_Automation.log)).
Before another visual run, correlate the visible popup with the fresh process
tree and preserve the new `AutoSDKInfo` output; do not disable platform
validation or change global Windows/.NET settings based on this correlation.

### Headless automation versus D3D12 Game startup (2026-10-09, 02:47–02:50 local)

An isolated headless `RavenPerch` automation run started at 02:47:51. Its log
recorded `UBT AutoSDK ReturnCode: -532462766` at 02:47:54, then completed
`CaptiveSky2.Agent.RavenPerch` successfully at 02:48:25 and exited normally.
This reproduces the code without preventing the focused test from passing.

A fresh D3D12 Game launch started at 02:49:07. At 02:49:12 it launched UBT
platform validation and a new bundled .NET 10 `dotnet.exe` appeared. The Game
log stopped at `TurnkeySupport`; the .NET process remained nearly idle, and
`AutoSDKInfo.txt` was stale from the prior successful run. After about 55
seconds without progress, only that Game launch and its newly created .NET
process were stopped. The interactive editor (PID 828) remained running.

The generic `0xe0434352` dialog was visible during the Game attempt, but Windows
exposed the window under `csrss.exe`; no direct owner, managed exception stack,
or matching recent .NET/Application Error/WER event was recovered. Together,
these runs strengthen the lead that the intermittent popup is associated with
UE's bundled UBT/Turnkey validation during standalone Game startup, but they do
not prove ownership or root cause. Preserve a process dump/child-process trace
and the fresh validation output on the next recurrence; do not suppress platform
validation based on this evidence.

Evidence: [`Codex_RavenPerchCheck_Automation.log`](../../Saved/CompileScratch/Claude_Props/Saved/Logs/Codex_RavenPerchCheck_Automation.log)
and [`Codex_MinnowEyes3_Game.log`](../../Saved/Playtests/Codex_MinnowEyes3_20261009/Codex_MinnowEyes3_Game.log).

A later read-only snapshot while the same dialog was visible found the
interactive editor (PID 828) but no new UBT/Game child. The only other
`dotnet.exe` processes had started on 2026-09-29 and 2026-10-01; their command
lines were unavailable. A 30-minute Application-log query returned no matching
.NET Runtime, Application Error, WER, exception-code, or UBT event. This
snapshot does not link the displayed dialog to either older process and does
not supersede the earlier Game-startup correlation.

### Bundled .NET host sanity check (2026-10-09)

The UE 5.8.3 bundled host completed `dotnet.exe --info` successfully in 1.7
seconds and reported SDK 10.0.203 with .NET runtimes 10.0.7. This verifies that
the host and SDK can start and enumerate their installation; it does not
exercise UnrealBuildTool, Turnkey, AutoSDK enumeration, or the standalone Game
startup path. It therefore argues against treating the generic popup alone as
evidence that the machine-wide .NET installation needs repair, while leaving
the UBT/Turnkey correlation open.

## Scope and next step

The exception investigation did not change Windows permissions or disable SDK
validation. A separate, scoped Raven attention source change and its scratch
build/test are recorded in
[`2026-10-08-raven-rain-basin-attention.md`](2026-10-08-raven-rain-basin-attention.md);
the connected editor's unsaved material was left untouched. Do not “fix” this
dialog by changing Windows ACLs or globally disabling SDK validation. If the
exact exception must be traced to its managed stack, correlate a visible popup
with the process and Event Viewer details, then inspect the corresponding fresh
UBT output. The current evidence supports an Unreal tooling/validation lead,
not an Aster/Raven gameplay exception.

Evidence: [`CurrentAsterGroundedWander.log`](../../Saved/Logs/CurrentAsterGroundedWander.log)
contains the matching return code and later successful game/probe shutdown;
`CurrentAsterMoveToListeningStones.log` records the same code in a second
isolated launch. The direct validation's requested workspace log was not
created.

### Isolated nest build and render (2026-10-09)

A sandboxed `Build.bat` invocation for the nest scratch project launched the
bundled `dotnet.exe` but remained at 0.12 CPU seconds for over a minute and did
not create its requested workspace log. Only that build was interrupted. This
matches the documented UBT per-user log-access stall in the restricted
execution context; it is not evidence of a source-level build error.

With approved elevated access to UBT's per-user log location, the same isolated
scratch target completed UHT, compiled, and linked successfully. The focused
`CaptiveSky2.Agent.IslandNest` automation passed, and the real-RHI
`CaptiveSky2.Visual.Viewpoints` capture passed for the nest close-up. The UE
startup logs still recorded `UBT AutoSDK ReturnCode: -532462766` on these runs,
while the editor continued through content load and the tests completed with
exit code 0. The scratch `AutoSDKInfo.txt` itself reported a successful
platform-validation result. This strengthens the correlation between UE's
bundled .NET/Turnkey path and the generic popup signature, but still does not
prove which process owns a visible dialog or provide its managed exception
type/stack. No Windows ACL, SDK configuration, or validation settings were
changed.

### Sandboxed UBT access exception reproduced (2026-10-09)

An un-elevated UBT build from the Codex workspace reproduced an unhandled CLR
exception with `System.UnauthorizedAccessException`: UBT could not enumerate
`C:\Users\freel\AppData\Local\UnrealBuildTool` while backing up its trace.
Retrying in UBT session mode bypassed that trace folder but then failed while
writing/reading
`C:\Users\freel\AppData\Local\UnrealEngine\Intermediate\Build\UnrealBuildTool.Env.BuildConfiguration.xml`.
Both are outside the project workspace and blocked by the command sandbox.

With an explicitly elevated build command, the same isolated target compiled
and linked successfully in 9.9 seconds. The focused Minnow automation passed,
and a bounded real-RHI Game probe exited normally with zero model requests. No
ACL, SDK-validation setting, or project configuration was changed. This gives
one reproducible source for a generic `0xe0434352`-style dialog during
sandboxed UBT use; it does **not** prove that every dialog seen in the user's
interactive Unreal session has this cause. If the popup recurs outside an
agent build, its owning process and matching Windows event are still needed.

Build output: [`Codex_RippleTint_VisualPolish_Build_20261009.log`](../../Saved/CompileScratch/Codex_RippleTint_20261009/Saved/Logs/Codex_RippleTint_VisualPolish_Build_20261009.log).
