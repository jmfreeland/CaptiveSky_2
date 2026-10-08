# Raven attends to rainwater (2026-10-08)

When the Listening Stones rain basin is wet enough to float a leaf, a grounded or
perched raven within 12 m can briefly turn its head toward the actual water
surface. The cue requires a clear visibility trace, has a 1.8-second eased
attention window, and fires once per wet spell. A dry or unplaced basin rearms
it. The bird does not move, drink, alter basin state, create a memory, or make a
model request. Higher-priority local cues remain in front of it.

The behavior uses the existing Raven attention loop and the basin's
renderer-independent water state. Basin floor thickness and water depth define
the look target so attention lands on the water instead of the stone rim. The
automation fixture verifies dry/wet eligibility, grounded versus flying,
surface targeting, head turn without body movement, non-repetition during a wet
spell, and rearming after the basin dries.

UE 5.8.3 `CaptiveSky_2Editor` rebuilt successfully in the isolated
`Codex_RavenCrabAttention_20261008` scratch project. Headless
`CaptiveSky2.Agent.RavenPerch` passed with NullRHI, Python and agent thinking
disabled, zero model requests, and isolated world data:
[`Codex_RavenRainBasinAttention_verify.log`](../../Saved/CompileScratch/Codex_RavenCrabAttention_20261008/Saved/Logs/Codex_RavenRainBasinAttention_verify.log).

This proves the deterministic attention rule, not that the saved Island's Raven
comes within range or that the glance is visually legible during ordinary play.
A short, bounded PIE/Game observation is the next validation; do not add a
thirst need or automatic basin visit without evidence that the current cue is
insufficient.
