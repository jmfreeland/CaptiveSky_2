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
- `CaptiveSky2.Agent.TidepoolMinnows` and
  `CaptiveSky2.Agent.TidepoolMinnowWaterPresence` both passed under NullRHI with
  agent thinking disabled and the model-request cap at zero. The focused test
  covers dry, high, and distant visitors; a near-water scatter and ripple; no
  repeated response while lingering; re-arming after the outer radius is
  crossed; and a grounded visitor crossing the Tideglass pool footprint.
- The original spring high/low assertion was over-precise: it compared absolute
  fish heights within 0.01 cm even though the dawn-rest curve intentionally
  changes the fish's tiny vertical swim bob. It now allows a 0.25 cm tracking
  error while the adjacent assertions still require the fish center to remain
  within 1.3 cm of the water surface at both tides. The tests have separate
  automation paths so either case can be selected independently.
- No real-RHI or in-editor swim-by has been reviewed yet; visual timing and
  readability remain unverified.

Logs: `Saved/Logs/Codex_MinnowTideCorrected_20261013.log` (tide/school pass) and
`Saved/Logs/Codex_MinnowWaterPresenceFinal_20261013.log` (focused pass).
