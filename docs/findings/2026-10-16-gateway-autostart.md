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

## Live recheck — 2026-10-17

The user-level `CaptiveSky Agent Gateway` task was re-registered from the
checked-in installer and read back as **Enabled**, **Running**, with an
`MSFT_TaskLogonTrigger`, `StartWhenAvailable=True`, and
`MultipleInstances=IgnoreNew`. Task Scheduler showed the interactive `freel`
principal. Its existing process chain was verified directly: hidden PowerShell
launcher PID 6536 -> `dotnet run` PID 23140 -> `CaptiveSky.Gateway.exe` PID
27696, all created on 2026-10-16 around 11:42. The gateway process responded,
and its .NET process had an established outbound TCP connection. The startup
transcript shows Raven's configured token environment variable and the LLM key
were present, and the readiness check passed; secret values are not logged.

An explicit `Start-ScheduledTask` request returned `0x800710E0` while the task
was already Running. Do not treat this as evidence that the gateway failed or
start a duplicate: the existing task-owned process chain and socket remained
active. The logon trigger is installed, but a fresh logon-triggered launch has
not been tested because that would require logging out/restarting the user's
session. A successful future logon is therefore configured, not yet directly
observed.
