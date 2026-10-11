# Persistent craft-tradition foundation (2026-10-11)

This is the first canonical-data slice toward the worldbuilding idea of Aster
refining a loaf and teaching the Innkeeper. `UIslandWorldStateSubsystem` now
has renderer-independent records for a craft's origin/source, bounded
normalized parameters, practice outcomes, and explicit teacher-to-learner
lessons. A learner can carry the lineage forward only after acquiring it from
a resident who already knows the tradition. Attempts require prior knowledge,
are limited to one per resident per Island day, and cannot backdate later
events. Writes use the existing atomic world-state save and roll back in-memory
on failure.

Bounds are deliberately small: 16 traditions, 32 lessons per tradition, 24
recent attempts per tradition, and 1–4 unique parameters in the normalized
range 0..1. World-state writes now emit schema version 2; version-1 files with
no craft array still load with an empty ledger. No resident action, model call,
bread asset, animation, or level placement is wired to these records yet. The
record layer alone does not make Aster craft or teach anything in play.

Craft IDs are limited to 48 ASCII letters/digits, `_`, or `-`, making them safe
exact references in a resident's prompt and social message.

`teach_craft` is now an optional field on the existing `speak` decision. The
brain lists only traditions that resident actually knows, and the existing
social component carries the exact ID through its normal reserved, in-range
conversation. The receiver records an attributed lesson only after delivery;
the existing four-line conversation cap and reply/cooldown pauses remain in
force. This uses an already scheduled thought/conversation turn, not a new
background request. It does not claim that a loaf or any other item was made
or handed over.

`IslandCraftTraditionTests.cpp` covers first practice, bounded traits,
slug-safe IDs, duplicate-day rejection, explicit chained teaching,
unauthorized practice, chronological order, bounded history, persistence, and
version-1 compatibility. `AgentSocialTests.cpp` additionally verifies the
prompt and parser fields, teacher-only knowledge visibility, delivery through
the bounded social path, rejection of invented IDs and out-of-range speech,
and that the queued reply starts no model request in the fixture.

The `CaptiveSky_2Editor Win64 Development` target built successfully on UE
5.8.3. `CaptiveSky2.Agent.IslandCraftTradition`,
`CaptiveSky2.Agent.CraftTeaching`, and `CaptiveSky2.Agent.SocialPacing` all
completed successfully in Editor-Cmd with thinking disabled, an isolated
playtest data root, a 60-second runtime cap, and a zero-model-request cap.

Next: add an optional resident practice action at a real, nearby craft setting
(initially the inn hearth), with grounded parameters and bounded attempts; then
create a small visible bread/board presentation as a separate asset/level
milestone. That is what will let an observed resident choice originate a
tradition in ordinary play. Do not pre-seed Aster's or the Innkeeper's memories
with a recipe before practice is implemented and verified.
