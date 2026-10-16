# Gateway logon autostart — 2026-10-16

Updated the existing `CaptiveSky Agent Gateway` user task and its installer.
The task is enabled with an at-logon trigger for `SHINYBOX\freel`, starts
when a scheduled start was missed, is permitted to start and continue on
battery, and ignores duplicate task instances. It retains the existing hidden,
limited-privilege PowerShell launcher and gateway restart wrapper.

Read-back from Task Scheduler confirmed those settings. The existing gateway
processes (PowerShell PID 6536, dotnet PID 23140, app PID 27696) were already
running since 11:42 and were left untouched; no duplicate was started. The
task reports `Running`, while `LastTaskResult` still contains the earlier
refusal code `2147946720` from before the settings update. A successful new
logon-triggered run has not yet occurred, so next-sign-in startup remains the
final verification. Gateway readiness had already passed without exposing
credentials; its current network connection was not changed by this update.
