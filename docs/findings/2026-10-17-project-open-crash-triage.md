# Project-open crash triage — 2026-10-17

The Windows `0xe0434352` application errors observed on 2026-10-16 are not, by
themselves, evidence of an Unreal crash. The corresponding `.NET Runtime` 1026
events name `dotnet.exe` / `CaptiveSky.Gateway.Checks.exe` and show unhandled
Gateway test failures: two assertion failures (17:31 and 20:30) and an
`UnauthorizedAccessException` while replacing a headless-turn marker (12:03,
12:04 and 17:21). These should be investigated as Gateway test/runtime issues.

A separate Unreal project-open failure is recorded at
`Saved/Crashes/_0000/CrashContext.runtime-xml`, with matching log
`Saved/Crashes/_0000/CaptiveSky_2.log` and run note
`Saved/Crashes/_0000/Codex_RatHollowTextureAudit_20261016.log`. The command
was a UE 5.8.3 `UnrealEditor-Cmd` project launch using
`-ExecutePythonScript=Scripts/Preview-GeneratedStaticMesh.py`; it crashed at
`SecondsSinceStart=0` with `EXCEPTION_ACCESS_VIOLATION` reading null. The
PCallStackHash is `F405610C114D120D4B6F5ABA901EC469AC9FAA26`. The dump reports
no OOM, and its stack contains only unsymbolized `UnrealEditor-Core` and
`UnrealEditor-Cmd` offsets. The requested `Saved/Logs` file was not created;
the crash-side log ends during engine pre-initialization.

This is distinct from the live editor PID 22688 that the user asked to leave
running; it does not justify terminating or reopening that editor. No build or
new Unreal process was started during the 2026-10-16 crash-log review.

## Repeat during a bounded Rat-hollow review (2026-10-17)

Before the attempt, no `UnrealEditor`, `UnrealEditor-Cmd`, UBT, or shader worker
process was present. A real-RHI `UnrealEditor.exe -game` review was then started
with agent thinking disabled, zero model requests, an isolated playtest data
root containing temporary Site A and Site B RatHollow records, and a 90-second
play cap. It terminated during pre-initialization before the Island loaded or
any screenshot was saved. `Saved/Crashes/_0000/CrashContext.runtime-xml` again
reports a null-address access violation at `SecondsSinceStart=0`, no OOM, and
PCallStackHash `4DCE2FEABCC3C947BBCB4179F07C8373B60F798D`—the same signature
recorded for the separate 2026-10-13 Game-mode startup failure. It is **not**
the hash of the earlier NullRHI project-open crash (`F405610C...`); matching
exception type and startup timing alone do not establish a shared fault. See
[`2026-10-13-unreal-startup-triage.md`](2026-10-13-unreal-startup-triage.md)
for the earlier distinction. The launch note contains the full command line;
the main Island state file was not used or changed. Do not repeat this launch
until symbols or a meaningfully different startup path can distinguish the
fault.

The isolated test records are not a canonical Rat home. The current
`WorldState/Island.json` still has an empty `wildlife_homes` array, and the Rat
subsystem therefore falls back to the Inn; the earlier Site A screenshot has a
`stat streaming` overlay and does not complete the two-site human/Rat-height
visual review. Keep the user's chosen “hidden fallen-wood hollow” intent, but
do not promote either candidate coordinate without readable visual evidence.

## Next diagnostic step

Before retrying any project-open automation, locate the exact UE 5.8.3 symbols
or a local minidump debugger and resolve the recorded stack. If symbols are not
available, compare the command-line launch environment and startup plugin/module
loading against a known-good project-open run. Do not infer that the absent
Trace Server, optional profiler DLLs, plant plugin retirement, memory pressure,
or the Gateway exceptions caused this null dereference; current evidence does
not establish any of those causes.

## Symbol/debugger availability recheck — 2026-10-17

The installed engine resolves to `D:/Games/Epic/UE_5.8`. A read-only inventory
found no `UnrealEditor*.pdb` beside its Win64 binaries and no `cdb.exe`,
`windbg.exe`, or `llvm-symbolizer.exe` available on the local command path or
in the checked Windows Kits debugger directory. The matching stack therefore
remains unsymbolized; this recheck does not identify a cause and does not
authorize another identical project launch.

## Repeat during the transient Norway Maple preview — 2026-10-17

After confirming no Unreal Editor, commandlet, UBT, or shader-worker process was
running, a one-instance Norway Maple D preview was attempted through the
existing transient `Preview-InstancedSkinnedMegaplant.py` path with
`-RenderOffscreen`. The process crashed during `StartOfEnginePreInit`, before
the script ran, before the Island loaded, and before any preview image was
written. The crash-side log is
`Saved/Crashes/_0000/Codex_InstancedSkinnedNorwayMapleD_20261017.log`; the
matching context reports a null-address read at `SecondsSinceStart=0`, no OOM,
and PCallStackHash `F405610C114D120D4B6F5ABA901EC469AC9FAA26`—the same
signature as the earlier UE 5.8.3 NullRHI project-open crash. No source asset,
Content asset, or Island map was changed. The engine-only `UnrealEditor-Cmd
-version` probe returned exit code 0 after about 55 seconds but printed no
version text; this is not a known-good project-open control. Do not retry the
Maple preview or another identical project launch until a symbolized stack or a
meaningfully different startup path is available.

## Startup-plugin comparison and symbol path — 2026-10-17

The current `.uproject` enables `ProceduralVegetationEditor` and `DynamicWind`;
both plugin folders exist in the installed UE 5.8 engine. DynamicWind was added
to the project on 2026-10-17 at 17:01 (`2389f2e`), to support the Megaplant
transform provider. However, the identical NullRHI project-open hash
`F405610C114D120D4B6F5ABA901EC469AC9FAA26` was already recorded on 2026-10-13,
before DynamicWind was enabled. The plugin change therefore cannot explain the
first occurrence and should not be disabled as a presumed fix. PVE was enabled
on 2026-10-10 and is present, but the available crash evidence does not isolate
it either.

The latest Maple preview still fails at `SecondsSinceStart=0`; its crash context
contains Core/Cmd module offsets and a minidump, but no local debugger or engine
symbols were found. Epic's UE 5.8 installation options provide an optional
**Editor symbols for debugging** component, intended to make engine crash
information more detailed ([installation options](https://dev.epicgames.com/documentation/en-us/unreal-engine/install-unreal-engine)).
If that component is installed for the matching 5.8.3 build, use the existing
minidump/crash context to resolve the current Core offsets before another
project-open run. This is a diagnostic prerequisite, not a proposed crash fix.

## Focused intertidal test launch — 2026-10-17

After correcting the quoting of command-line values containing spaces, a
NullRHI scratch launch requested only
`CaptiveSky2.Agent.TidepoolPeriwinkleIntertidalZonation`. It still terminated
before the requested `Saved/Logs/IntertidalZonation_20261017.log` was created,
so the automation test itself did not run and has no pass/fail assertion result.
The crash-side launch record is
`Saved/CompileScratch/Codex_VisitorResonance_20261017/Saved/Crashes/_0000/IntertidalZonation_20261017.log`;
the corresponding `CrashContext.runtime-xml` reports a null-address access
violation at `SecondsSinceStart=0` and PCallStackHash
`F405610C114D120D4B6F5ABA901EC469AC9FAA26`, matching the earlier UE 5.8.3
project-open failures above. Thinking, Python, and ModelContextProtocol were
disabled, the agent model-request budget was zero, and the play cap was 120
seconds; none of those settings advanced startup far enough to exercise the
intertidal behavior. Do not repeat this launch unchanged. The periwinkle source
change is compiled but remains runtime-unverified until the startup stack can
be resolved or a meaningfully different known-good project-open path is
established.

## Live editor handle and linker lock — 2026-10-18

The current Windows capture still returned Edge when targeting the listed
`CaptiveSky_2 - Unreal Editor` window; one fresh `activate_window` attempt timed
out after about 10 seconds. Although PowerShell did not enumerate an
`UnrealEditor` process, an elevated UE 5.8.3 build gave authoritative evidence
that `D:\Games\Epic\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe` is still
holding the project module
`Binaries\Win64\UnrealEditor-CaptiveSky_2.dll`. UBT waited 20 seconds for the
DLL, then failed at the link step with Windows' file-in-use error. No source
compilation or successful relink was reported, and the editor was not killed.

Treat this as an inaccessible, likely unresponsive live editor—not a confirmed
process exit. Do not retry the editor-target link or overwrite the loaded DLL
until the editor has exited through a controlled route. The user previously
asked to leave the unresponsive editor running, so wait for their direction
before closing it; project-open preview attempts also remain gated on resolving
the known Core startup crash. This does not explain that crash or establish
whether the editor window itself is responsive.
