# Gateway logon autostart — 2026-10-16

Updated the existing `CaptiveSky Agent Gateway` user task and its installer.
The task has an at-logon trigger for `freel`, starts when a scheduled start was
missed, is permitted to start and continue on battery, and ignores duplicate
task instances. It retains the existing hidden, limited-privilege PowerShell
launcher and gateway restart wrapper.

After a later elevated re-registration, Task Scheduler read-back confirmed the
task is enabled and `Running`, with an interactive, limited-privilege principal,
`StartWhenAvailable`, and `IgnoreNew` duplicate handling. This supersedes the
earlier query's stale `LastTaskResult` refusal code; no gateway process was
manually launched and no duplicate was started. The task's registration and
current running state are verified, but a successful fresh-logon-triggered
start is not yet verified. Gateway readiness had previously passed without
exposing credentials; this update did not change its network connection.
