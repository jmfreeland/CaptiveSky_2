# Raven notices Wind Arch gust motes (2026-10-08)

When grounded or perched, the raven can notice the nearest visible Wind Arch
mote within 14 m if an unobstructed visibility trace reaches it. It gives the
gust one brief, eased head glance (about two seconds) without moving, creating
a memory, or making a model request. Flying and resting ravens ignore this cue,
and the same gust actor cannot restart the glance. A listening-stone chime
retains higher attention priority.

The UE 5.8.3 `CaptiveSky_2Editor` scratch build succeeded with
`-NoHotReloadFromIDE` while the main editor was open. The
`CaptiveSky2.Agent.RavenPerch` automation passed headlessly with agent thinking
disabled, model requests capped at zero, and the test process capped at 60
seconds. The fixture covers mote range, grounded-only response, a head turn,
stationary body, easing, and no repeat glance for one gust. Local log:
[`Codex_RavenWindMote_20261008.log`](../../Saved/CompileScratch/Codex_RavenCrabAttention_20261008/Saved/Logs/Codex_RavenWindMote_20261008.log).

This verifies the behavior in the automation fixture, not its readability or
timing in ordinary play. The next useful check is a short PIE observation near
the Wind Arch; no frame-rate measurement or visual review was made here.
