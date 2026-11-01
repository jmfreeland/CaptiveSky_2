# Recurring Unreal Zen Storage startup delay (2026-11-02)

The user reports that Unreal's “Zen Storage Server is slow to launch” warning
appears on every editor start. The available worktree logs confirm that slow
starts do occur, but do not yet explain the reported frequency:

- `Codex_ResidentFeedback_TideFix_20261101_1230.log` reports Zen initialization
  taking 29.793 seconds. Unreal found an existing server on port 8558 with a
  different run context, requested a clean shutdown, launched the installed
  server, waited for readiness, then reported HTTP status `OK`.
- `Codex_ResidentFeedback_ModelTier_20261031.log` reports a 27.31-second
  initialization.
- In the other inspected launches, Unreal reused the existing server and
  initialization generally took less than one second.

This is evidence of recoverable slow startup in some runs, not a persistent
Zen failure. The successful status and DDC connection came after each observed
delay. The “different run context” restart is a plausible contributor in the
first log, but the available evidence does not prove it is the root cause or
that it explains every interactive editor launch. A `zen.exe service status`
check reporting that the Windows `ZenServer` service is not installed is not
by itself a failure: Unreal normally manages its local Zen server with the
editor rather than requiring a separately installed Windows service.

Do not delete the Zen data directory, reinstall the server, or change DDC
configuration based on these logs alone. On the next occurrence, preserve the
current editor's `Saved/Logs/CaptiveSky_2.log` and record the elapsed time.
Compare the `LogZenServiceInstance` sequence from “Launching zen utility” to
“Local ZenServer AutoLaunch initialization completed”, especially whether it
reuses a server, shuts one down for a context mismatch, or waits repeatedly for
readiness. Then inspect the same-run Zen server log and Windows storage/health
signals before proposing a repair. Do not restart an editor with unsaved work
just to reproduce the warning.

The historical workspace logs are local ignored playtest artifacts and may
not be present in a clean checkout; the timing and observations above are the
durable evidence summary.
