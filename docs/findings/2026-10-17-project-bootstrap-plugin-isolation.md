# Project bootstrap with the MCP plugin isolated — 2026-10-17

The project-opening NullRHI automation attempt previously crashed before the
project log or test run, with
`PCallStackHash=F405610C114D120D4B6F5ABA901EC469AC9FAA26`. The project's
`.uproject` enables `ModelContextProtocol` alongside the separate engine plugin
`MCPClientToolset`.

In the ignored `Codex_VisitorResonance_20261017` scratch project, a UE 5.8.3
editor target linked successfully (35 actions, 72.52 s). Launching the same
scratch project with only `-DisablePlugins=ModelContextProtocol`, NullRHI,
thinking disabled, zero model requests, and `-NoWorldState` passed the focused
`CaptiveSky2.Agent.FireflyCorridorPlacement` automation test and exited with
code 0. The engine's separate `MCPClientToolset` remained mounted. This is the
first project-opening automation run that reached a test after the earlier
bootstrap crashes, and it makes the project MCP plugin a strong suspect; it is
not yet a controlled proof of causality, since this was not a paired retry with
the plugin enabled under identical cache/session conditions.

A separate run in the same plugin-disabled scratch configuration reached
`CaptiveSky2.Agent.NightEcology` but failed four existing shore-crab player
presence assertions (startle, direction, cooldown, re-arm). No firefly
placement assertion failed. The narrow test isolates and verifies the new
firefly placement behavior; the broader ecology fixture issue remains open.

The main `.uproject`, enabled MCP plugin, and saved map were not modified. PID
22688 was observed unresponsive after the scratch build and absent from the
process list after the focused test; this work did not terminate it. Keep the
project's MCP plugin enabled for normal editor use; use the isolated disable
flag only for bounded headless tests. Next:
inspect the four crab fixture failures, then make a controlled comparison of
project startup with and without `ModelContextProtocol` only when the main editor
is no longer holding the project open. A successful NullRHI automation test does
not constitute a visual or real-RHI play review.
