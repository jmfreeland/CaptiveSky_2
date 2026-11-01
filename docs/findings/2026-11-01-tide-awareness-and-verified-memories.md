# Tide awareness and verified interaction memories

## Evidence and changes

In the isolated 15-minute resident session, Rat and the Innkeeper separately
asked to understand how the shoreline changes as the tide falls. Tideglass
already supplies a bounded tide-height cue, so resident perception now also
describes whether the water is flooding, ebbing, or turning at high/low water.
Review found that Tideglass had been using a 24.84-hour curve while the Island's
sea description used a 12.42-hour semidiurnal tide. Tideglass and the actual
WaterBodyOcean height now share the Island clock's normalized tide function,
including spring/neap variation. Tide look-ahead also advances the day at
midnight. The ocean uses the Water plugin's height-offset API instead of moving
its actor or water bounds; updates are rate-limited and the authored offset is
restored on world teardown.

The same session exposed a memory-truth bug. At 10:19:52 UTC Raven's RainBasin
interaction was rejected as a recent repeat inspection. The isolated basin
state (`Saved/CC/Project/Saved/Playtests/Codex_ResidentPacing_20261101/WorldState/Island.basin.json`)
contains no leaves, but Raven's memory at that timestamp said, “After rain
filled the stone basin, I set a leaf afloat on its water.” The model memory was
written while parsing the decision, before the controller executed it.

The prompt now explicitly says the chosen action has not yet happened, and
decision parsing does not store model-proposed memories on an `interact`
response. A later decision may remember the controller's verified result. A
regression case checks that a proposed RainBasin success memory is not stored;
existing idle-reflection storage remains covered.

## Validation and remaining uncertainty

The tide-curve consistency and midnight assertions, ocean-height restoration
case, resident-perception checks, and premature interaction-memory regression
are in the automation suite. The UE 5.8.3 isolated editor target compiled
successfully after correcting a numeric-limit sentinel and updating ecology
tests to use the shared semidiurnal-period constant (87.6 seconds, 39 actions).
The focused commandlet attempt crashed before engine/test startup
(`SecondsSinceStart=0`, null access violation, hash
`F405610C114D120D4B6F5ABA901EC469AC9FAA26`), matching the recurring headless
bootstrap failure; no automation assertions ran in that attempt. A later,
separate bounded Game-mode pass verified the tide descriptions and resident
memory behavior in live simulation; it did not run those automation assertions
or validate rendered appearance.

The first attempt to start the 15-minute isolated NullRHI resident-feedback
session through the PowerShell script was rejected before Unreal launched
because script execution is disabled. The local policy was not overridden. A
subsequent direct launch used the same built-in play-time and model-request
caps, an isolated data root, and no rendered RHI; its evidence is below.

The full-hour flood/ebb language and transition wording also need a rendered
play check. No rendered run was started: device-wide GPU headroom is 3.1 GiB,
below the project's 4 GiB gate. Canonical Island state and the open editor were
not changed.

## Isolated 15-minute resident-feedback Game pass — 2026-11-01 12:25 UTC

The scratch `Saved/CC/Project` was launched once as Game/Spectator with
`-NullRHI`, Python disabled, situation logging enabled, a fresh isolated
`DataRoot`, `-CaptiveSkyMaxRealtimeSeconds=900`, and
`-CaptiveSkyMaxModelRequests=32`. It reached play safety in about 62 seconds,
ran 900.3 real seconds, then exited normally with 16 requests and none in
flight. The main Island data root and editor were not used; no rendered output
or visual-quality claim follows from this run.

The residents' world context tracked the shared tide through the session: Raven
initially chose to watch the pool as the tide turned, and later context
described the near-high waterline and ebb. At the optional feedback invitation
after 739–741 real seconds, Aster and Raven chose silence and were left free to
do so. Rat and the Innkeeper independently chose to set leaves afloat after
the basin was full and each had approached it. Both controller outcomes
reported success; the isolated basin save contains exactly those two leaves.

Most importantly, Rat's and the Innkeeper's `memory.jsonl` entries were written
at the respective verified outcome timestamps, after the controller's
successful `RainBasin` interactions. There is no proposed success memory
written ahead of an attempted action. This directly verifies the earlier
memory-truth fix in live Game simulation, beyond the compile-only automation
regression. The decision log, isolated chronicle, basin save, and per-resident
memories remain available under
`Saved/CC/Project/Saved/Playtests/Codex_ResidentFeedback_TideFix_20261101_1230/`.
