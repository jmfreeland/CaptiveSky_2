# Unreal editor restart recheck (2026-10-31)

The user confirmed that editor changes were saved and approved restarting to
apply Nanite Foliage. The open `CaptiveSky_2 - Unreal Editor` window was closed
with Alt+F4. Its existing project log records `Alt-F4 pressed!`,
`QUIT_EDITOR`, world cleanup, and `Editor shut down`; no save prompt appeared.

The project was then opened through its `.uproject` association. Unreal created
a process whose window title was `The Game has crashed and will close`, then
exited before writing a fresh project log. A subsequent editor-only launch also
failed to expose a targetable window. The existing `Saved/Logs/CaptiveSky_2.log`
still ends at the normal shutdown timestamp (2026-10-31 14:20:00); no new
project-local crash context was produced by these attempts.

This does not identify the cause and is not evidence that Nanite Foliage loaded
after restart. No map, source, project setting, or crash-report contents were
changed or dismissed. Keep the editor restart and `r.Nanite.Foliage=1` readback
unverified. Next: reopen the project from a responsive desktop, inspect any
visible crash details before dismissing them, then verify a new startup log and
the live console value before running a bounded visual check.
