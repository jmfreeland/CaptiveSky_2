# UnrealBuildTool .NET startup exception (2026-10-08)

## Finding

The dialog reports `dotnet.exe - Application Error` with exception
`0xe0434352`. During an isolated UE 5.8.3 Game launch, the engine logged:

```text
LogTargetPlatformManager: UBT AutoSDK ReturnCode: -532462766
```

`-532462766` is the signed 32-bit representation of `0xe0434352`. The
matching return code identifies UnrealBuildTool's AutoSDK/platform-validation
child process as a concrete source of this dialog in the tested launch. The
engine continued loading, initialized the Island, and completed a bounded
resident movement probe; this was not a gameplay-process crash. This strongly
connects the screenshot to Unreal's startup tooling, but does not prove that
every dialog the user has seen came from the same invocation.

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
