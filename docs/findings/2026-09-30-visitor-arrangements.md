# Visitor-made stone arrangements (2026-09-30)

Human visitors can now take part in the same small, lasting creative practice as
Island residents. At an arranging ground, **E** opens a focused panel where the
visitor can choose a ring, line, spiral or pair; add an optional title and personal
meaning; or, at another maker's work, place a small arc of response stones. The
existing world-state records, weathering, resident perception and response limits
are reused rather than creating a parallel visitor-only system.

The shape and age become part of the shared landscape. A maker's title and meaning
remain visible only to that maker unless they choose to share them. A visitor is
identified as `Visitor`, can contribute only once per Island day across all sites,
and is never allowed to overwrite their own work. Submission rechecks that the
visitor can still reach and clearly see the same site; failed saves do not claim a
lasting result. The reversible prototype reset remains `Island.ForgetArrangements`.
No LLM/provider calls or session/request budget changes are involved.

Implementing the visitor path exposed a first-session presentation gap: when the
Island first created arranging-site records, it saved their locations but did not
spawn the four visible ground actors until later edits. First-time site placement
now spawns all four empty sites immediately, so they can actually be seen and used
in that first session.

UE 5.8.3 Editor build succeeded. `CaptiveSky2.Agent.IslandArrangement` passed with
coverage for first-session site actors, the bound E target/open path, a visitor
creating a spiral and then responding to another maker's ring on the next Island day,
persistence into a new world, private title/intent, visible stone count, and the
one-contribution-per-day limit across both actions. Log:
`Saved/Logs/Codex_VisitorArrangements_Final.log`.

The automation fixture does not render the Slate panel, so its visual placement,
keyboard focus and actual mouse/touch clicks remain to be checked in rendered Play or
PIE. The next useful pass is a brief, bounded visual interaction check, followed by
confirming residents can notice and optionally respond to a visitor's work in live
play without prompting them or implying they know its private meaning.
