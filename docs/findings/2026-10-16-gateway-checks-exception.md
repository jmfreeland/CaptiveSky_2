# Gateway check-process CLR exception — 2026-10-16

## Windows event evidence

The Application log contains two unhandled CLR exceptions at 12:03:55 and
12:04:31. Both name
`Gateway/tests/CaptiveSky.Gateway.Checks/bin/Debug/net8.0/CaptiveSky.Gateway.Checks.exe`,
not `dotnet.exe`, UnrealBuildTool, or UnrealEditor. The exception is
`System.UnauthorizedAccessException` while `HeadlessTurnMarker.MarkActiveAsync`
atomically replaces `headless-turn.json` with `File.Move(..., overwrite: true)`.
The stack proceeds through the gateway's `AgentMessageRouter` and the test
program's route check. The Application Error records the same checks executable
and CLR status `0xe0434352`.

In this environment, the existing checks executable reproduces the same
`File.Move` access denial when run sandboxed, even with its temporary test root
under the project. Running the same executable with approved local filesystem
access passes every gateway check. This is evidence that the two check-process
events are consistent with sandbox restrictions on atomic rename; it is not a
gateway production failure and does not justify weakening the marker's atomic
write.

## Distinguish from the desktop `dotnet.exe` dialog

Separate Windows Error Reporting events at 13:31, 15:01, and 15:31 are
`RADAR_PRE_LEAK_64` reports whose problem signature names `dotnet.exe` (version
10.0.726.21808). They are leak-detection reports, not the `.NET Runtime` 1026
or `Application Error` 1000 events found above. The recent Application log
does not establish that these reports caused the user's recurring
`dotnet.exe - Application Error` dialog during Unreal work. The checks
executable's path and fault offset also do not match the dialog's reported
application name and exception location. Treat the visible dialog as still
unattributed; capture its owner PID/command line and the matching event at the
time it appears before changing Unreal, the SDK, or the gateway.

## Verification

- Gateway checks in the restricted sandbox: stopped at the first headless
  marker rename with `UnauthorizedAccessException` (`0xe0434352`).
- Same existing checks executable with approved local access and its test data
  rooted under `Saved/CompileScratch`: all checks passed, including message
  routing, proposal parsing/review-only behavior, duplicate handling, and
  request caps.
- No gateway source or persistent state was changed by this investigation.
