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

### Latest elevated validation (2026-10-09, 09:47 local)

The denser crossing-site automation rebuilt in the isolated `Codex_NestFoundation_20261009` scratch project. The elevated `Build.bat` invocation ran UE's bundled .NET 10 / UnrealBuildTool successfully; editor startup recorded `UBT AutoSDK ReturnCode: 0`, and the refreshed `AutoSDKInfo.txt` said `Result: Succeeded` (0.21 s). Windows SDK 10.0.22621.0 was found. Setup warnings for Android, iOS, Linux, macOS, TVOS, LinuxArm64, and VisionOS reflect unavailable optional platform SDKs; they did not prevent the Win64 editor target from building or loading `/Game/Maps/Island`. `CaptiveSky2.Agent.CrossingSiteAudit` then passed and the commandlet exited normally. The desktop UI was not observable from this tool session, so whether a separate modal appeared could not be verified.

This confirms the controlled elevated path is healthy and can validate the project; it does not identify the owner of the user's separate popup or establish that every occurrence is caused by sandboxed UBT. Continue to avoid un-elevated UBT invocations that cannot access its documented per-user trace/config paths, but do not change Windows permissions, uninstall SDKs, or disable validation. If a popup appears outside one of our restricted build attempts, capture its process tree at that moment before attributing it.

Build/test evidence: [`Codex_CrossingSiteAudit_Dense_20261009.log`](../../Saved/Logs/Codex_CrossingSiteAudit_Dense_20261009.log); refreshed scratch SDK report: `Saved/CompileScratch/Codex_NestFoundation_20261009/Saved/Logs/AutoSDKInfo.txt`.

### Bounded Game rerun with the validation return code (2026-10-09, 13:18–13:20 local)

An isolated D3D12 Game launch again logged `UBT AutoSDK ReturnCode:
-532462766`. Startup also reported that it could not launch the Zen version
utility from the user-profile install directory because process creation was
denied, and it fell back to its legacy version check. Despite those warnings,
the Island loaded, the delayed 120-frame CSV command was accepted, and the
play-session watchdog ended the run after 45.1 real seconds with zero model
requests; deinitialization logged 45.2 seconds and the engine exited normally.

This is another example of the validation return code coexisting with a
successful bounded Game session, not proof that the displayed `dotnet.exe`
dialog came from that process. A 15-minute Application-log query after the run
found no matching `.NET Runtime` or `Application Error` event, and no
`dotnet.exe`, UBT, or Unreal process remained. The dialog's owner and managed
exception stack are still unknown. No SDK, Windows permission, or validation
setting was changed.

Evidence: [`Codex_MoveWarm_20261009.log`](../../Saved/CompileScratch/Codex_VegetationLayerAudit_20261009/Saved/Playtests/Codex_MoveWarm_20261009/Codex_MoveWarm_20261009.log); [`Profile(20261009_131932).csv`](../../Saved/CompileScratch/Codex_VegetationLayerAudit_20261009/Saved/Profiling/CSV/Profile%2820261009_131932%29.csv).

### Focused automation startup correlation (2026-10-09, 14:34–14:36 local)

Two fresh UE 5.8.3 commandlet startups for `CaptiveSky2.Agent.BlockedGroundMoveApproach`
and `CaptiveSky2.Agent.SessionSafety` each logged
`LogTargetPlatformManager: UBT AutoSDK ReturnCode: -532462766`, the same signed
code as `0xe0434352`. Both automation tests then completed successfully and
the commandlets exited through `Automation Test Queue Empty`; the first
asserted the blocked-move interaction fallback, and the second checked the
zero-request and real-time safety behavior. No gameplay or automation failure
followed the AutoSDK return code.

This repeats the code during a controlled startup and strengthens the
association with UE's bundled .NET/Turnkey AutoSDK path. It still does not
prove that this process owned the screenshot's dialog, identify the managed
exception type, or show that the same issue occurs in the user's interactive
editor. The latest Application-log query again found no matching
`.NET Runtime`/`Application Error` event with a managed stack. Continue to
capture a live process tree if the popup recurs; do not disable validation or
change Windows ACLs based on the code alone.

Evidence: [`BlockedGroundMoveApproach.log`](../../Saved/CompileScratch/Codex_VegetationLayerAudit_20261009/Saved/Tests/Codex_LandmarkApproach_20261009/BlockedGroundMoveApproach.log)
and
[`SessionSafety.log`](../../Saved/CompileScratch/Codex_VegetationLayerAudit_20261009/Saved/Tests/Codex_LandmarkApproach_20261009/SessionSafety.log).

### Latest scratch Game and safety regressions (2026-10-09, 16:31–16:41 local)

A fresh NullRHI scratch Game launch for the read-only Raven branch-support scan
again logged `UBT AutoSDK ReturnCode: -532462766`. The Island loaded and the
scan completed (53 visibility samples; no supported branch point), so the
return code did not stop that runtime work. The process did not end within its
150-second outer bound and was stopped by its exact process ID after the scan;
the session log has no managed exception type or stack trace. The existing
`AutoSDKInfo.txt` in that scratch directory was older than this launch, so its
earlier `Result: Succeeded` cannot be used as the result of this validation
attempt.

Two subsequent UE 5.8.3 NullRHI commandlet runs each logged the same return
code, then passed `CaptiveSky2.Agent.SessionSafety` and
`CaptiveSky2.Agent.BlockedGroundMoveApproach` respectively, with exit code 0.
They verify the request/time safeguards and collision-aware staging behavior
still pass on the current scratch build; neither exposes the AutoSDK child's
managed exception.

This is another controlled correlation to UE's UBT/Turnkey startup path, not
proof that it owns the user's visible `dotnet.exe` dialog. The screenshot's
generic code and address alone still cannot establish the process or stack.
The safe next diagnostic remains capturing the process tree and fresh UBT
output while the popup is actually visible; no validation settings, SDKs, or
Windows permissions were changed.

Evidence: [`Codex_RavenBranchAudit_Runtime.log`](../../Saved/CompileScratch/Codex_VegetationLayerAudit_20261009/Saved/Logs/Codex_RavenBranchAudit_Runtime.log),
[`Codex_Continuation_CaptiveSky2_Agent_SessionSafety.log`](../../Saved/CompileScratch/Codex_VegetationLayerAudit_20261009/Saved/Tests/Codex_Continuation_CaptiveSky2_Agent_SessionSafety.log),
and
[`Codex_Continuation_CaptiveSky2_Agent_BlockedGroundMoveApproach.log`](../../Saved/CompileScratch/Codex_VegetationLayerAudit_20261009/Saved/Tests/Codex_Continuation_CaptiveSky2_Agent_BlockedGroundMoveApproach.log).

### Repeated popup reported during a later turn (2026-10-10, 00:50 local)

The user supplied another screenshot of the same generic `dotnet.exe -
Application Error` dialog with exception `0xe0434352` and address
`0x00007FF841A2483A`. At inspection time, no `dotnet.exe`, UBT, AutomationTool,
or Unreal process was running. The Application log had no matching `.NET
Runtime`, `Application Error`, or WER record in the preceding 24 hours, so this
occurrence still cannot be mapped to a managed exception or executable owner.
The isolated basin automation had just completed successfully; its log does
not establish that it caused or owned the desktop dialog. The code and address
alone remain insufficient to attribute the popup.

### User-reported trigger pattern (2026-10-10)

The user reports that the dialog consistently occurs while Unreal is building
or running, but cannot identify the exact moment within that workflow. This
narrows the useful capture window to Unreal build/startup/runtime activity and
fits the repeated historical correlation with UE's bundled .NET/UBT Turnkey
platform-validation child. It still does not prove the popup belongs to that
child: the managed exception type, stack, and dialog-owning process remain
unobserved. On the next occurrence, preserve the dialog and correlate the
active Unreal/dotnet process tree with the fresh engine and UBT logs; do not
change validation, SDK, or Windows permission settings based on this pattern
alone.

To avoid needing to catch the exact moment, the opt-in
[`Watch-UnrealDotnet.ps1`](../../Scripts/Watch-UnrealDotnet.ps1) watcher samples
Unreal/UBT/dotnet process IDs and command lines once per second and records
matching Application error events to a local JSONL file under
`Saved/Diagnostics/`. Start it in PowerShell before opening/building Unreal with
`./Scripts/Watch-UnrealDotnet.ps1`; it runs for up to two hours or until Ctrl+C.
It changes no system settings. This is process/event correlation, not a crash
dump: a very short-lived process or an error absent from the Application log
can still escape capture. Review the local log before sharing it.
The script passed PowerShell syntax parsing, but its runtime smoke test was
blocked because the current PowerShell execution policy disables loading
scripts. No process-scoped or persistent policy override was attempted, so
runtime logging remains unverified until the user-approved execution method is
available.
