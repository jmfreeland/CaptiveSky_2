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
