# Tidepool crab shore-forage cue — 2026-10-29

## Change

Tidepool crabs now occasionally investigate a nearby, materialized storm-wrack item. The cue uses a local actor query every 0.8 seconds, a 240 cm notice radius, and a 500 cm rearm radius. A visit produces a three-second, eased movement detour capped at 75 cm, followed by a 14-second cooldown. The crab does not pick up, alter, or save the item; the behavior issues no model requests.

Visitors still startle the crab and cancel the forage cue. Night shelter also cancels it, so waking does not resume a stale target. The existing rain, tide, ripple, and raven responses are unchanged.

## Verification

- UE 5.8.3 `CaptiveSky_2Editor` target compiled and linked successfully on 2026-10-29 (14 UBT actions), including `IslandTidepoolCrab.cpp` and the new `IslandTidepoolCrabWrackTests.cpp`.
- The focused automation fixture covers one nearby find, no repeated cue while the item remains nearby, interruption by a visitor, rearm after leaving the wider ring, and shelter cancellation.
- The automation was not run because the current editor is unavailable and recent headless Unreal launches have crashed before test startup. Compilation is not evidence that the runtime behavior passed.

## Follow-up

Run `CaptiveSky2.Agent.TidepoolCrabShoreForage` in a working editor, then watch a bounded Tideglass session to confirm that the short detour reads naturally at game camera distance and never crosses into the pool or water.
