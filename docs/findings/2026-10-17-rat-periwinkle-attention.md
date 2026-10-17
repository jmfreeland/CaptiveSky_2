# Rat notices tidepool periwinkles

## Change

An otherwise idle rat may briefly turn toward the nearest visible tidepool
periwinkle within 4.2 m. It scans on a 1.2-second interval, holds the glance for
2.2 seconds, and waits 24 seconds before another snail glance. Existing Raven
and Listening Stones attention cues take precedence. The rat does not move,
interrupt an action or rest, request a model response, or persist a discovery.

This adds a small, legible cross-species moment at the rocky shore without
changing the snail's own independent tide, weather, or disturbance behavior.

## Regression coverage

`CaptiveSky2.Agent.RatPeriwinkleAttention` covers range, visibility, active
actions, rest, cooldown, and invalid distance. The test translation unit was
compiled as part of the editor target.

## Validation

The isolated UE 5.8.3 `CaptiveSky_2Editor` Development target compiled and
linked successfully through UBT (7 actions, exit code 0). A headless attempt to
run the focused automation test exited without creating its requested log, so
the test result is unverified. No live visual playtest was performed; the
desktop connection does not expose the Unreal window. This behavior makes no
LLM requests and does not change resident memory or the Island map.
