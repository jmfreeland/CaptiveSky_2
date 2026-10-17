# The rat listens when the Listening Stones chime

## Change

When an active Listening Stones chime is audible nearby, an idle rat may turn
toward it and briefly play its existing look-around animation. The response
uses the chime's attenuation radius, lasts 1.7 seconds, and has a 16-second
cooldown. It does not move the rat, interrupt travel or rest, make a model
request, or persist a discovery. If the rat is already attending to the raven,
the chime waits for that short response to finish and is only noticed if it is
still audible.

This gives the rat a small, legible relationship with a world landmark and a
natural event, while preserving the existing Raven glance and agent autonomy.

## Regression coverage

`CaptiveSky2.Agent.RatListeningStonesAttention` checks that only a live nearby
chime can begin the response, and that busy/resting states, cooldown, out-of-
range cues, and invalid distances suppress it.

## Validation

The isolated UE 5.8.3 `CaptiveSky_2Editor` Development target compiled and
linked successfully (UBT exit code 0). The focused automation test and live
visual behavior were not run: the desktop connection does not expose the
Unreal window, and the full-content scratch automation launch has previously
crashed before test startup with the F405610C signature. The Island map and
resident memory were not changed.
