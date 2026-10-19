# Common Frog only startles from a visible Rat (2026-10-19)

The Common Frog already startles from an awake nearby Rat, but the cue used
distance and height alone. A Rat could therefore trigger a hop through the
fallen-wood hollow or another solid obstacle. The Rat-specific cue now also
requires a clear `ECC_Visibility` trace between the frog's eye point and the
Rat's upper body. Sleeping, distant, or vertically separated Rats remain
non-threatening, and the existing one-visitor latch and cooldown are unchanged.

The pure predicate regression covers awake/visible, sleeping, occluded, distant,
and out-of-height cases. UE 5.8.3's existing isolated scratch target compiled
both `IslandCommonFrog.cpp` and `IslandCommonFrogTests.cpp` and linked the editor
module successfully (10 actions). The automation test was not executed, and the
cue has not had an in-game visual review; no project process was launched, and
the saved Island was not modified.
