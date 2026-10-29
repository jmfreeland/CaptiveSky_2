# Raven notices a crab's shore activity

The Raven's existing short crab-awareness cue now recognizes two local states:

- A crab briefly scurrying after a disturbance.
- A crab making its bounded detour to investigate nearby storm wrack.

The cue uses the existing wildlife scan and head-turn: the Raven must be grounded or
perched, awake, within 700 cm horizontally and 320 cm vertically, and not already
showing another attention cue. Each continuous crab-activity episode can prompt one
1.5-second glance. The latch clears when the crab is no longer scurrying or foraging,
allowing a later episode to be noticed. The Raven does not move, chase, alter the
shore-find, save state, or make an LLM request.

`CaptiveSky2.Agent.RavenPerch` now covers both scurry and forage attention, no
repeated restart during one episode, re-arming for a later episode, and stationary
Raven behavior.

## Verification

- UE 5.8 editor-target scratch build succeeded (66 actions, including
  `RavenAgentAIController.cpp` and `RavenPerchTests.cpp`).
- Automation result is pending: a commandlet invocation returned without a dedicated
  test log, so no pass is claimed.
- In-editor visual confirmation is pending because the current editor window is at a
  higher Windows integrity level than the available screen-control helper.

Separately, the same editor restart loaded `/Game/Maps/Island.Island`; its fresh log
set `r.Nanite.Foliage` to `1`, matching the saved `bEnableNaniteFoliage=True` project
setting. This confirms activation, not a performance improvement; FPS and VRAM still
need a controlled comparison.
