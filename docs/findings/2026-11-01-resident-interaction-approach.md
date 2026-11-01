# Bounded approach after an explicit resident interaction choice — 2026-11-01

## Evidence

In the isolated 15-minute resident-feedback run, Rat explicitly chose the
RainBasin interaction while still about 17 metres from the basin. The controller
rejected it with `Too far away to inspect; move within four metres first.` The
decision record contains the intended leaf interaction, but the basin state
remains at 0.989 water with no leaves. This was a failed approach, not evidence
that wet-basin leaf placement itself is broken.

## Change

An explicit interaction request for a visible, stationary target may now start
an approach when it is farther than the 4 m interaction limit but no farther
than 25 m. The existing grounded landmark navigation, capsule-clearance and
route checks remain in force. Wildlife are categorically excluded, so this
cannot turn observation into chasing. The interaction is still separate from
movement: it happens only after a successful arrival (or a blocked route that
nevertheless stopped within clear inspection range), where the controller
rechecks target visibility, range, cooldown and the target's current state.
Failed or interrupted movement produces no interaction side effect.

The focused resident-movement automation now covers the distance and target
eligibility limits, wildlife exclusion, interaction after a successful arrival,
no interaction after a blocked approach, and preservation of a real path failure
when navigation cannot start. The UE 5.8.3 isolated Editor target compiled and
linked with these cases. The automation has not been executed: the worktree's
known NullRHI startup crash remains unresolved, and no validation editor is
currently running. This is not yet a replay of Rat's basin interaction.

## Next check

When a validation editor safely loads this worktree module, run the focused
`CaptiveSky2.Agent.BlockedGroundMoveApproach` automation and a short isolated
wet-basin feedback pass. Confirm Rat approaches the basin, only sets a leaf once
within 4 m, and leaves a durable author/day entry. Keep that run isolated from
the canonical world-state directory. Rendered play remains gated on at least
4 GiB free GPU memory and the existing 30 FPS floor.
