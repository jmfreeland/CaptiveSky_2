# Tideglass dragonfly and frog response

## Change

When a common frog hops within 230 cm and 250 cm vertical separation, a nearby
Tideglass dragonfly briefly scatters away. The response checks at a staggered
0.45-second interval, has a five-second cooldown and a per-frog rearm latch, and
does not interrupt an active visitor, ripple, flower, or rain-basin cue. It is
transient local movement only: no model request, persistent memory, attack, or
world-state write is added.

## Verification

The new `CaptiveSky2.Agent.Ecology.DragonflyFrogResponse` automation test covers
the inclusive distance boundary, vertical separation, settled/distant frogs,
and invalid input. UE 5.8.3 built the `CaptiveSky_2Editor` target in an isolated
no-content scratch project, and the focused test passed under NullRHI. The
scratch run logged expected missing-asset warnings because it intentionally
had no project Content; it did not load the Island or exercise a natural
encounter. The user's open Unreal editor was left untouched, and this session's
desktop bridge exposes no app window.

An additional `CaptiveSky2.Agent.Ecology.DragonflyFrogResponseWorld` test also
passed in that scratch target. Its synthetic collision floor lets a real
`AIslandCommonFrog` start a hop; it then verifies the dragonfly scatters away,
latches the same hop against repeated startles, and preserves an active flower
visit. This is actor-level behavior evidence, not a real-Island visual review.

## Next visual check

Watch a daytime frog and one dragonfly together at normal player scale. Confirm
the short sideways dart reads as a startle and that routine frog hops do not
make the Tideglass group look constantly evasive.
