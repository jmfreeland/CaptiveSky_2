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
no craft array still load with an empty ledger. No bread asset, animation, or
level placement is wired to these records yet. The optional hearth-practice
action below now lets a resident originate or revisit a tradition through its
normal decision cycle.

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

The optional practice_craft action is offered only while a resident is within
normal interaction range and line of sight of the tagged, lit InnHearth. The
hearth subsystem independently rechecks the real tagged anchor, visibility,
range, and lit state before recording. New experiments require a bounded ID
and concise name/source; an existing tradition must already be known by that
resident and keeps its established identity. One to four intended settings
are accepted from the small baking vocabulary hydration, rise, heat, and salt,
each normalized to 0..1. They are speculative notes, not measured ingredients
or a success score; every hearth-created attempt is stored as Unrated, once
per resident per Island day. No ingredients or finished prop are spawned,
and a saved record is not described as a successful loaf. The controller
records the result in the resident's memory. This action uses the current
thought only; it adds no background request.

`IslandCraftTraditionTests.cpp` covers first practice, bounded traits,
slug-safe IDs, duplicate-day rejection, explicit chained teaching,
unauthorized practice, chronological order, bounded history, persistence, and
version-1 compatibility. `AgentSocialTests.cpp` additionally verifies the
prompt and parser fields, teacher-only knowledge visibility, delivery through
the bounded social path, rejection of invented IDs and out-of-range speech,
and that the queued reply starts no model request in the fixture.
IslandInnHearthTests.cpp verifies JSON parsing and controller dispatch for an
actual resident at the lit nearby hearth, persistence and memory attribution,
same-day deduplication, and rejection of a wrong target, dark hearth,
unsupported setting, and out-of-range practice.

The `CaptiveSky_2Editor Win64 Development` target built successfully on UE
5.8.3. `CaptiveSky2.Agent.IslandCraftTradition`,
`CaptiveSky2.Agent.CraftTeaching`, and `CaptiveSky2.Agent.SocialPacing` all
completed successfully in Editor-Cmd with thinking disabled, an isolated
playtest data root, a 60-second runtime cap, and a zero-model-request cap.
After adding the in-world action, the CaptiveSky_2Editor Win64 Development
target rebuilt successfully and CaptiveSky2.Agent.IslandInnHearth passed under
the same zero-request safeguards. These are isolated automation fixtures, not
evidence of a naturally chosen experiment in a live Island session.

Next: create a small, readable bread/board presentation as a separate
asset/level milestone and give the practice interaction a visible, gentle
performance at the hearth. The current action records a resident's choice
immediately and honestly, but has no hand-motion animation, modeled
ingredients, or visible food. Verify an optional resident's chosen first
practice and later teaching in a short, explicitly bounded live sample before
deciding whether that visual should be session-only or persist. Do not pre-seed
Aster's or the Innkeeper's memories with a recipe.
