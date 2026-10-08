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

## Scope and next step

No game source, user project config, Windows permissions, or editor state was
changed. Do not “fix” this by changing Windows ACLs or globally disabling SDK
validation. If the exact exception must be traced to its managed stack, rerun
the validation in an approved context that can access UBT's standard log
directory (or first arrange an approved UBT log-path override), then inspect
the fresh UBT log and event details. The current evidence supports an Unreal
tooling/permission-context issue, not an Aster/Raven gameplay exception.

Evidence: [`CurrentAsterGroundedWander.log`](../../Saved/Logs/CurrentAsterGroundedWander.log)
contains the matching return code and later successful game/probe shutdown;
`CurrentAsterMoveToListeningStones.log` records the same code in a second
isolated launch. The direct validation's requested workspace log was not
created.
