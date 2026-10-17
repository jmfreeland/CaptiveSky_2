# Dragonflies investigate a visitor's swim wake

## Change

Tideglass dragonflies now consider the player's short `PlayerSwimImpact` wake
alongside rain-, wind-, and minnow-made ripples. A morph close to the wake
briefly hovers above a small offset from the disturbance, using its existing
1.8-second interest animation and seven-second per-morph cooldown. This
extends the pond's visible cause-and-effect chain: swimming can draw a nearby
school's attention and a dragonfly's, while each resident still responds
independently.

The response is local, temporary, and non-destructive. It does not pursue the
player, change a dragonfly's home, or make a model request. The existing
interest arbitration remains in force: a ripple can interrupt a flower visit,
and a new cue cannot stack during the cooldown.

## Regression coverage

`CaptiveSky2.Agent.NightEcology` now checks that an untyped ripple is ignored,
a nearby tagged player wake triggers the short hover, the existing cooldown
prevents retriggering, and a minnow surface break still produces the established
response.

## Validation status

The isolated `CaptiveSky_2Editor` development target compiled the changed
dragonfly and ecology-test translation units and linked successfully (UBT exit
code 0; 52.58 seconds). The focused automation test has not been run; the
current UE 5.8.3 scratch project-open path previously crashed before tests
began with the F405610C startup signature. See
[`2026-10-17-project-open-crash-triage.md`](2026-10-17-project-open-crash-triage.md).
