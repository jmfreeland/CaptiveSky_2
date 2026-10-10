# Recurring `dotnet.exe` exception during Unreal work (2026-10-10)

The reported Windows dialog names `dotnet.exe` and exception `0xe0434352`.
That code identifies a managed exception but the displayed address alone does
not identify its type or origin. No matching `.NET Runtime` / `Application
Error` event or retained `dotnet.exe` crash dump was available in the recent
Windows Application log or WER report folders inspected on this date.

UE 5.8.3 explicitly uses its bundled .NET 10 SDK for UnrealBuildTool, at
`D:\Games\Epic\UE_5.8\Engine\Binaries\ThirdParty\DotNet\10.0\win-x64\dotnet.exe`.
The system `dotnet` is a separate .NET 10.0.302 installation at
`C:\Program Files\dotnet\dotnet.exe`. The observed project builds and editor
automation invoked the bundled host. Four editor builds completed successfully
after fixing a test-only access-control issue; four real-D3D12 editor-world
wildlife captures passed, and the process watcher observed their clean exits.
No matching managed exception event appeared during those observed runs. This
does not rule out an intermittent failure outside the observation window.

Separate Unreal failure evidence was found in the 2026-10-10 06:03 crash report:
it is an `UnrealEditor` Game-mode assert, not a `dotnet.exe` exception. The
installed `Installed` Derived Data Cache graph had no writable node. Unreal
reported that the configured common cache under
`C:/Users/freel/AppData/Local/UnrealEngine/Common/DerivedDataCache` was
read-only (its access check returned Windows error 5); Zen cache startup also
reported access denied and a failed server-version copy. That report is
consistent with a separate cache-permission/configuration problem during
launch, but it does not identify the cause of the managed exception shown in
the Windows dialog. A later isolated Game launch supplied the project's
`Saved/LocalDDC` explicitly and completed successfully. Future comparison
should record whether the failing launch uses the common cache or an explicit
writable DDC path.

There is separate machine-level evidence of a graphics-kernel failure: Windows
logged bugcheck `0x00000113` with an unexpected reboot on 2026-10-09. This may
be relevant to Unreal graphics stability, but there is not enough evidence to
connect it to the `dotnet.exe` dialog.

## Next diagnostic step

If the error recurs, use a narrowly scoped local WER dump for the bundled
`dotnet.exe` so the CLR exception and stack can be identified. A full process
dump can contain secrets or other in-memory data; do not enable it without the
user's approval. The repository's `Scripts/Watch-UnrealDotnet.ps1` can record
process paths, command lines (with common secret fields redacted), and matching
Application events during an observed build/run window, but it cannot recover a
managed exception stack by itself.
