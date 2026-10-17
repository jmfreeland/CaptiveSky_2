# Minnows scatter at an awake rat's shoreline approach

## Change

The Tideglass minnow school now checks every 0.4 seconds for a nearby Rat
controller. An awake rat within 5.5 m horizontally and 3.5 m vertically can
briefly scatter the school through its existing 2.4-second response. The cue
fires once per approach; the rat must retreat beyond 8.5 m before it can
re-trigger, and a separate 8-second cooldown limits rapid re-approaches.
Sleeping rats do not disturb the fish. No new actor, asset, persistent state,
or model request is added; the existing startle ripple cooldown still applies.

## Regression coverage

`CaptiveSky2.Agent.TidepoolMinnows` now covers an awake shoreline rat, sleeping
state, horizontal and vertical bounds, and invalid distance.

## Validation

The isolated UE 5.8.3 `CaptiveSky_2Editor` Development target compiled and
linked successfully (final retry: 4 actions, exit code 0). The focused
automation source compiled during that build sequence, but its test was not
run: the full-content scratch automation startup has a known pre-test
null-access crash, and the live Unreal window is not exposed for a visual
playtest. The behavior is transient and does not affect session/request caps.
