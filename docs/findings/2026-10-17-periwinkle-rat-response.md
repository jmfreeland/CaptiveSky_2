# Periwinkle tucks at a close rat approach

## Change

The tidepool periwinkle now briefly tucks when a rat comes within 1.9 m and
within 1.2 m vertically. It reacts once per approach; the rat must retreat
beyond 5.2 m before a later approach can trigger another tuck. The existing
short observation cooldown is respected. This complements the rat's longer-
range, idle glance at the snail without making either animal move or assigning
them an intention.

## Regression coverage

`CaptiveSky2.Agent.TidepoolPeriwinkleRatCue` covers the inclusive close-range
boundary, farther approaches, large vertical separation, and invalid distances.
The automation translation unit compiled with the editor target.

## Validation

The isolated UE 5.8.3 `CaptiveSky_2Editor` Development target compiled and
linked successfully (10 actions, exit code 0). The focused automation test was
not run: previous full-content scratch test launches crashed before automation
startup, and the latest headless invocation did not create its requested log.
No visual playtest was performed because the Unreal window is not exposed in
this session. The behavior adds no model calls or persistent world changes.
