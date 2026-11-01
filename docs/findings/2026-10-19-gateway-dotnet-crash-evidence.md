# Recorded .NET dialogs: Gateway checks, not proven Unreal crashes

Windows Application-log events from 2026-10-13 through 2026-10-18 contain
several `0xe0434352` managed-exception dialogs. The matching `.NET Runtime`
events identify `CaptiveSky.Gateway.Checks.exe` or `dotnet.exe`; the recorded
stack enters `HeadlessTurnMarker.MarkActiveAsync`, where
`File.Move(temporary, path, overwrite: true)` threw
`UnauthorizedAccessException`. Other events are `InvalidOperationException`
assertion failures from the Gateway check program. None of the matching events
inspected names `UnrealEditor.exe`, `UnrealBuildTool`, or Epic Games Launcher as
the faulting application. This is evidence about the recorded dialogs, not
proof that no separate Unreal/UBT failure occurred.

The current Gateway check suite was rebuilt and run on 2026-10-19 with output
under the ignored `Saved/CompileScratch/GatewayChecks_20261019/bin/` directory
and `CAPTIVESKY_GATEWAY_CHECKS_ROOT` set to
`Saved/CompileScratch/GatewayCheckData_20261019/`. All checks passed, including
proposal parsing and review safeguards. The run emitted a non-fatal NuGet
vulnerability-feed warning because `api.nuget.org` was unreachable. Redirecting
the check data root avoided the recorded marker-path failure; the suite did not
contact Discord or the production request inbox.

An isolated UE 5.8.3 source build also succeeded on 2026-10-19, but no project
automation/runtime launch was performed in this check. If another dialog
appears during an Unreal build or launch, capture its timestamp and matching
`.NET Runtime` or `Application Error` event before attributing it to Unreal;
this event-log sample does not identify the separate project pre-init crash.

## 2026-10-28 event-log recheck

Elevated, read-only Windows Application-log queries found paired events at
22:08:20 and 22:09:48–49. The `.NET Runtime` 1026 entries and `Application
Error` 1000 entries identify `CaptiveSky.Gateway.Checks.exe` (CoreCLR/.NET 8),
not Unreal or `dotnet.exe` as the faulting application. Both stacks terminate
on `System.UnauthorizedAccessException` in
`HeadlessTurnMarker.MarkActiveAsync` while moving the temporary marker over
`headless-turn.json`. These are repeat observations of the known test-harness
path/permission issue; they do not prove that the screenshot's separately
named `dotnet.exe` dialog was this event.

The same event-log interval includes distinct Unreal native crashes: an
`UnrealEditor-Core.dll` access violation (`0xC0000005`, offset `0x2950F9`) at
21:09 on 2026-10-28, an unknown-module `0xC0000005` at 10:40 on 2026-10-29, and
an `UnrealEditor-CaptiveSky_2.dll` `0xC0000005` at 13:11 on 2026-10-29 (offset
`0x38923C`). Those are native access violations, not the managed
`0xE0434352` marker exceptions. Keep the two classes separate; a future
desktop dialog still needs timestamp/PID correlation before assigning it to
either.
