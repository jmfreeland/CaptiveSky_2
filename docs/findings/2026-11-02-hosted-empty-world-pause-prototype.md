# Dedicated-host pause when empty (2026-11-02)

The hosted-empty-world decision now has an opt-in engine path. Launch a
dedicated server with `-CaptiveSkyPauseWhenEmpty` to enable it; local, editor,
and listen-server play are unchanged. The project Blueprint GameMode derives
from `ACaptiveSky_2GameMode`, which supplies the lifecycle hooks.

At map initialization, resident model admission is fenced before resident
actors begin their first thought. After `BeginPlay`, an empty dedicated host
atomically checkpoints the Island clock and wetness, then pauses the game
world. If a visitor disconnects later, `Logout` checks the remaining server
player count, fences resident work, checkpoints, and pauses. On `PostLogin`,
the host unpauses before reopening resident work. A world-session generation
is captured for thought and sleep-consolidation requests; a late response is
still charged to the request budget but cannot append memory, change
personality, or produce an action across a disconnect/reconnect boundary.
An interrupted sleep consolidation stays in its current state while empty and
finishes without applying the stale response after a visitor returns.

The `CaptiveSky2.Agent.SessionSafety` fixture now covers the dedicated-only,
explicit-opt-in policy and session-generation fencing; the clock persistence
fixture exercises the public final-checkpoint method. The saved-map ocean
fixture also asserts that the configured Blueprint GameMode inherits the new
lifecycle hooks. The UE 5.8.3 Editor target compiled and linked successfully
with the isolated `Saved/CS58` project; the final safety-gate build completed
seven incremental actions.

This is a private-host prototype, not a public-cloud deployment. It counts
network players only: authenticated account policy and active external-channel
visitor leases (including Discord) are not connected to this lifecycle yet.
It pauses the Unreal world but does not stop the host process. The existing
bounded watchdog continues to use process wall time; a hosted continuous
instance still needs service-level runtime and spending quotas. If the clock
checkpoint fails, the host logs the error and pauses at the last durable
snapshot. If Unreal refuses the pause request, it logs an error and keeps
resident model work fenced, but other world simulation may continue. The
host also rejects new joins after a failed/missing clock checkpoint until an
operator restarts or recovers the saved state. Runtime automation and an actual
disconnect/reconnect pass remain unverified;
do not enable this on a public host until those paths and the failure cases are
tested.
