# Common Frog notices a low dragonfly

## Behavior

The Common Frog now occasionally turns toward a nearby Tideglass dragonfly when
it is visible, 35–280 cm away horizontally, and 45–220 cm above the frog. A
staggered 0.55–0.9 second poll and a visibility trace keep this a lightweight,
local ecology cue. One pass holds attention for 1.4 seconds, follows the
dragonfly's current position, and then eases back to the frog's prior heading.
A 16-second cooldown and an outer presence latch prevent repeated turning at
the same insect. Hopping and existing startle behavior take priority. This
adds no actor, persistent state, model request, or map edit.

## Verification

UE 5.8 built the isolated `Codex_DragonflyFrogResponse_20261019` editor target
successfully. The focused `CaptiveSky2.Agent.Ecology.CommonFrogDragonflyAttention`
automation test passed under NullRHI, checking the distance/height bounds,
trigger, turn, return-to-heading, and cooldown/latch behavior. The test uses a
transient world, not the saved Island.

The scratch project does not contain the Island's imported Common Frog and
foliage assets, so this proves the movement/attention logic with engine fallback
geometry only. It does not prove that the turn reads clearly at normal play
distance, that the visibility trace behaves as intended among the Island's
plants, or that there is no measurable cost in a full habitat. Review it in a
bounded zero-model-request Tideglass pass before treating it as visually
approved; retain the 30-FPS floor.
