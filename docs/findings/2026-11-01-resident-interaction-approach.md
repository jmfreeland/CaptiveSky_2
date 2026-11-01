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
known NullRHI startup crash remains unresolved. The main CaptiveSky_2 editor is
open and responsive in the Island map, but it is the separate project editor
and has not loaded this worktree's module; it cannot validate this change yet.
The current whole-device GPU sample is 9,601 / 12,282 MiB used (2,681 MiB
free), below the 4 GiB rendered-play gate, so no PIE/Game session was started.
This is not yet a replay of Rat's basin interaction.

## Guidance consistency follow-up

The resident system prompt and basin's own out-of-range description now agree:
moving to `RainBasin` first remains the recommendation, while an explicit
interaction choice may start the existing bounded approach and is rechecked
only within interaction range. The basin world fixture now covers the observed
17 m target distance and rejects automatic approach at 26 m; it also checks the
updated resident and basin guidance. The UE 5.8.3 scratch Editor target compiled
and linked all three changed translation units. These new assertions have not
been executed because the previously recorded NullRHI startup crash is still
unresolved. No editor window was restarted or used to validate the module.

## Next check

When a validation editor safely loads this worktree module, run the focused
`CaptiveSky2.Agent.BlockedGroundMoveApproach` automation and a short isolated
wet-basin feedback pass. Confirm Rat approaches the basin, only sets a leaf once
within 4 m, and leaves a durable author/day entry. Keep that run isolated from
the canonical world-state directory. Rendered play remains gated on at least
4 GiB free GPU memory and the existing 30 FPS floor. The live viewport faces
offshore and does not show the generated woodland, so it also cannot validate
the vegetation changes.

**Commandlet retry (2026-11-01 11:38 UTC):** a focused
`CaptiveSky2.Agent.IslandRainBasinWorld` run used the isolated scratch project,
NullRHI, `-NoZen`, memory DDC, disabled resident thinking, zero model requests,
and `-TestExit="Automation Test Queue Empty"`. Unreal again crashed before
initialization (`SecondsSinceStart=0`, null access violation, hash
`F405610C114D120D4B6F5ABA901EC469AC9FAA26`); no automation test was discovered
or run. The crash context and startup log are under
[`Saved/CC/Project/Saved/Crashes/_0000`](../../Saved/CC/Project/Saved/Crashes/_0000/Codex_RainBasinWorld_20261101.log).
