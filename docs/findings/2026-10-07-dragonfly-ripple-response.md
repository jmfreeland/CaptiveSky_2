# Dragonflies notice natural pool ripples (2026-10-07)

Daytime Tideglass dragonflies now briefly arc toward nearby wind- or rain-made
surface ripples, as if investigating the movement on the water. They hover above
the ripple rather than landing, keep their normal wind-aware patrol, and return
to it after 1.8 seconds. Each response has a seven-second cooldown, and only the
existing `WindImpact` and `RainImpact` tags qualify; visitor-created ripples are
ignored. The behavior is transient and adds no saved state, model requests, or
new actors.

UE 5.8.3 `CaptiveSky_2Editor` built successfully (11 actions). The focused
`CaptiveSky2.Agent.NightEcology` automation passed with exit code 0, covering
visitor-ripple filtering, natural-ripple detection, hover height, cooldown, and
movement toward the ripple. The test ran in an isolated `-NullRHI` automation
world with memory-only DDC; it did not start a gameplay session. Log:
[`Codex_DragonflyRippleRetry_20261007.log`](../../Saved/Logs/Codex_DragonflyRippleRetry_20261007.log).

The source test is not a substitute for a close in-world visual check. The
interactive editor was not visible in this session, so the swoop's appearance
and gameplay frame cost remain unverified.

## Distinct hover stations (2026-10-07)

The three daytime color morphs previously aimed at the exact same point above a
natural ripple. They now use three fixed, 120-degree-separated hover stations
on a 42 cm radius around the ripple, while keeping the same 110 cm water
clearance, 1.8-second interest window and seven-second cooldown. The response
remains transient and does not create actors or persistent state.

The `CaptiveSky2.Agent.NightEcology` regression now checks the hover radius and
that all three color variants can investigate the same ripple without stacking
on the same point. A rendered close-up is still needed to judge whether the
small separation reads naturally in play.
