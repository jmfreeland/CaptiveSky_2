# Tideglass minnow surface skim (2026-10-17)

The Tideglass school already emitted a restrained ripple when one fish broke the
surface, but the fish itself stayed on its ordinary underwater orbit. A calm,
rain-free surface break now gives that same fish a short 14 cm arc and a slight
upward tip over 0.54 seconds. The motion eases back to the existing orbit; the
other four fish, school footprint, cooldowns, and resident context are
unchanged. The existing rain/startle gates still decide whether a surface break
may happen.

`IslandMinnowTests` now checks that only the ripple's source fish is selected,
that the fish clears the waterline at the arc's midpoint, and that an actor tick
expires the hop and restores its normal orbit and pitch.

Validation: the UE 5.8.3 `CaptiveSky_2Editor` target compiled and linked in the
existing `Saved/CompileScratch/Claude_Props` project, which junctions `Source/`
to the current worktree and keeps its output binaries separate from the open
editor. The incremental build compiled `IslandMinnowTests.cpp` and linked the
module successfully. Automation did not run: the current project commandlet is
known to crash during Core startup before test discovery (same startup failure
recorded in the October 17 coordination notes). In-world timing and readability
still need review in the editor or a real play session.
