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

### Residents remember public stone work

When a resident sees a completed arrangement, they now add its exact site to their
private remembered-places file under a neutral, form-only label (for example, `a stone
ring`). While the work is visible, it is not redundantly offered as a remembered
destination; after the resident walks out of sight, the exact site tag is available as
an optional return target. Empty grounds are not kept as discoveries. This deliberately
does not copy the maker's title or intent into another resident's memory.

The UE 5.8.3 editor target built successfully, and
`CaptiveSky2.Agent.IslandArrangement` passed with assertions for return-from-memory,
privacy, and no remembered target for empty ground. The fixture uses unique agent IDs
and cleans their place-memory directories. Log:
`Saved/Logs/Codex_ArrangementMemory_Final_20260930.log`. This validates the
perception/memory path without LLM calls; naturally choosing to return in a live session
and the visual appearance remain unverified.

### Response-field keyboard focus

The panel focuses the title field for a new arrangement and the intent field when opening
another maker's work. A repeated-open test exposed a handoff race: the game-and-UI input
mode targeted the panel root, which could take focus from the selected field. The controller
now makes the panel visible and sets its state first, then targets the eligible edit field
as the input-mode focus widget (falling back to the panel only when no contribution is
available). The focus fixture forwards the real input-mode call and checks both field
choices while preserving the existing create/respond flow. The scratch test also exposed
unnecessary manual `UnPossess` cleanup on a transient AI controller; destroying the fixture
world already cleans it up and avoids a commandlet teardown crash.

The UE 5.8.3 scratch-linked module compiled and both `CaptiveSky2.Agent.IslandArrangement`
and `CaptiveSky2.Agent.IslandArrangementInspection` passed. Log:
`Saved/CompileScratch/Codex_ArrangementFocus_20261002/IslandArrangement_Fixed.log`.
This verifies the Slate/input-mode handoff in the fixture; panel placement at game resolution
and mouse/touch behavior still need a rendered PIE check.
