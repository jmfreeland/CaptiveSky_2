# The woodland stag acknowledges a quiet visitor — 2026-10-13

The stag already gave brief, non-startled looks to a calm resident or nearby
fox, but treated the player as absent. It now checks the first player pawn at
the same 0.35-second cadence used for its other local awareness cues. A slow,
visible visitor within 6 m earns the existing look-around animation; the latch
rearms after leaving 8.5 m, and a separate 8-second cooldown prevents rapid
repeat looks.

This is a sensory cue only: the stag does not approach, flee, persist a memory,
or request model inference. Sleeping, moving, startled, or otherwise occupied
stags ignore the cue. It uses one first-player query and, for an eligible
visitor, one short visibility trace per awareness check; no extra world scan is
introduced.

Verification: the UE 5.8.3 editor target built, and
`CaptiveSky2.Agent.WoodlandDeer` passed with NullRHI, thinking disabled, Python
disabled, zero model requests, a 60-second cap, and isolated world state. The
runtime tick passes the first player controller's pawn into the awareness
check; the regression supplies a fixture pawn directly because the headless
test world does not possess player characters. It covers visible slow
approach, animation, no-movement behavior, one-shot latch, leave/reapproach
cooldown, and the sleeping-stag guard. A bounded real-RHI Game observation
remains useful to confirm the look reads naturally at visitor scale.

Automation log: [`Codex_StagVisitor_20261013.log`](../../Saved/Logs/Codex_StagVisitor_20261013.log).
