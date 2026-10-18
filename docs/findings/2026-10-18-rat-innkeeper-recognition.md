# Rat–Innkeeper quiet recognition

The Rat can briefly turn toward the configured Innkeeper when they are nearby
and have a clear line of sight. It is a small acknowledgement, not a greeting
action: it does not move either resident, interrupt the Innkeeper, write
memory, or create an LLM request. The Rat must be awake and idle, the
Innkeeper must not be resting, and a 28-second per-Rat cooldown prevents a
constant stare. Existing Raven, Listening Stones, and periwinkle attention
keeps priority; the new cue yields while one of those responses is active.

The recognition uses the Innkeeper subsystem's configured stable agent ID, so
it follows configuration rather than a display-name guess. A 1.4-second local
scan keeps the behavior bounded. The pure eligibility predicate is covered by
`CaptiveSky2.Agent.RatInnkeeperAttention` for visibility, action/rest state,
range, cooldown, and invalid distance.

## Validation

`git diff --check` passed. The UE 5.8.3 isolated scratch target
`Codex_VisitorResonance_20261017` built and linked successfully (10 actions;
both the controller and automation-test translation units compiled). The
focused automation did not run: the full-Content commandlet launch still has
the known Core null-access startup crash recorded in `COORDINATION.md`; do not
repeat that launch until the startup issue is resolved. Normal-play readability
also remains unverified. The editor is visible to the desktop inventory but
`activate_window` timed out twice, so it was left running. Once project startup
is healthy and the editor is responsive, run
`CaptiveSky2.Agent.RatInnkeeperAttention`, then observe the Rat near the Inn
with thinking disabled and zero model requests. No Content or map assets were
changed.
