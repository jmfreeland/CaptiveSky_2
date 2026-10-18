# Unreal project startup crash: metadata-path clue

## Evidence

The latest captured project-opening crash is `Saved/Crashes/_0000`, timestamped 2026-10-17 19:45:19. Its context reports an access violation reading address zero, `SecondsSinceStart=0`, and `UnrealEditor-Cmd` frames; no Island map, gameplay, or project log reached world-ready. The corresponding command line is preserved in `Saved/Crashes/_0000/Codex_InstancedSkinnedNorwayMapleD_20261017.log`. The process got through `StartOfEnginePreInit`, began engine startup, and attempted to start `UnrealTraceServer.exe`; the executable was missing and Unreal fell back to in-memory tracing.

The crash context's `PCallStack` hash is `F405610C114D120D4B6F5ABA901EC469AC9FAA26`. It has offsets but no function names. I compared those offsets with the export table of the installed UE 5.8.3 `UnrealEditor-Core.dll` (no symbols were available). The nearest exported functions, in stack order, include `FOutputDevice::LogfImpl`, `FWindowsPlatformMisc::SetStoredValue`, `FGenericPlatformMisc::GetMachineId` / `GetLoginId`, `FCsvProfiler::Get`, `FCsvProfiler::SetMetadata`, and `FLowLevelMemTracker::ProcessCommandLine`.

## Interpretation and limits

This is an export-table proximity match, **not symbolized stack analysis**. It suggests the failure is in early engine command-line / profiling-metadata startup, before level loading, rather than in a placed Island actor or the new ecology behavior. The missing trace-server binary is an observed warning, not evidence that it caused the null read. `UEMinidump.dmp` exists (306,008 bytes), but the matching UE PDBs and `cdb.exe` in the Windows SDK's expected location were absent, so the exception instruction and exact caller could not be resolved. Do not attribute this crash to the Island map, a specific plugin, or the platform-identity functions yet.

This is separate from the managed `0xE0434352` UBT/Turnkey failures recorded in the October 8 highlight note; that HRESULT-like exception code does not explain this native access violation.

## Next diagnostic gate

Do not repeat the same project launch with the same command line. First obtain matching UE 5.8.3 symbols (or another usable dump debugger). Then reproduce once with a known-idle desktop, a bounded timeout, a unique log path, and the smallest meaningful change to tracing/metadata startup. Compare the resulting symbolized stack and whether the crash occurs before or after `StartOfEnginePreInit`. Keep the user's editor and unsaved work untouched.

## Current desktop check — 2026-10-18

The desktop inventory reports the UE 5.8 editor as running with both a
`CaptiveSky_2 - Unreal Editor` window and a `Message Log` window. Two attempts
to activate the project window timed out. A screenshot-backed state request for
that window showed the Edge desktop instead of the editor, so it did not provide
a trustworthy viewport capture. The editor was left untouched; this is evidence
of failed UI access, not by itself proof that the editor process is frozen.

A separate, no-project `UnrealEditor-Cmd.exe -version` invocation on this date
produced no version text in the captured output and remained live after roughly
20 seconds; no explicit log path was supplied. Only that newly started command
was interrupted. This differs from the earlier recorded successful engine-only
version check; the discrepancy does not identify the project-open crash's
cause. Avoid repeating either launch until the engine-only startup or desktop
capture path behaves reliably, or a usable debugger/symbol set becomes
available.
