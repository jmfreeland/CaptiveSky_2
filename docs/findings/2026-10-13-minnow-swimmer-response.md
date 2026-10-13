# Tideglass minnows notice a nearby swimmer

## Change

The Tideglass minnow school now checks the local player at 5 Hz. A nearby
surface swimmer can startle it, and a player crossing the measured Tideglass
water footprint can do so even though that custom shallow surface is not the
ocean Water plugin's swim volume. Both cues require the player to be within 5 m
horizontally and 2.5 m vertically of the school. A close approach produces the
existing brief, bounded scatter and one restrained surface ring. The school
stays inside its shallow-water footprint and returns to its ordinary orbit. A
visitor must leave an 8.5 m outer radius before the school notices a new
approach; an eight-second cooldown also prevents rapid re-entry from stacking a
response. A dry visitor beside the pool, distant swimmer, or visitor well above
the shallows is ignored. This adds no memory, world-state change, or model
request.

## Verification

- UE 5.8.3 `CaptiveSky_2Editor` target built successfully.
- `CaptiveSky2.Agent.TidepoolMinnows.PlayerWaterPresence` passed under NullRHI
  with agent thinking disabled and the model-request cap at zero. It covers dry,
  high, and distant visitors; a near-water scatter and ripple; no repeated
  response while lingering; re-arming after the outer radius is crossed; and a
  grounded visitor crossing the Tideglass pool footprint.
- The broader existing `CaptiveSky2.Agent.TidepoolMinnows` test executed but
  reports one failure in its earlier spring high/low tide position assertion
  (`IslandMinnowTests.cpp:243`). The focused swimmer test is separate so this
  tide assertion does not obscure the new interaction. The tide discrepancy
  remains to be diagnosed independently.
- No real-RHI or in-editor swim-by has been reviewed yet; visual timing and
  readability remain unverified.

Logs: `Saved/Logs/Codex_MinnowWaterPresence_20261013.log` (focused pass) and
`Saved/Logs/Codex_MinnowSwimmer_20261013.log` (broader-suite tide failure).
