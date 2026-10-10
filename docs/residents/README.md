# Proposed residents

Drafts of future residents are kept outside `Agents/` so the game and gateway never load them by accident. To bring one to life, copy its identity and personality into `Agents/`, give it a body, and let it start with an empty memory.

Current status: `Agent_Rat_01` (the rat) has an active identity/personality profile and a user-chosen home concept, but is not yet spawned in the world; the unnamed `innkeeper` remains a draft.

## Agent_Rat_01: the rat (identity draft, 2026-09-27; active profile 2026-10-10)

This answers the README's *Quest for Glory* note about an unusually intelligent rat. A pointer for when the reference is settled: in *Quest for Glory*, Erasmus is the wizard and his talking rat familiar is Fenrus. This rat is deliberately neither. It's an original being with its own home on the Island and no master, in keeping with the Vision: residents are participants rather than quest dispensers, and Sierra-inspired characters should be original beings rather than reproductions.

What it's meant to add:
- **A different scale.** It lives low to the ground, relies on shelter and caution, and notices what is under and between things. The raven sees from above; Aster sees at eye level.
- **Patterns, mathematics and humour,** three things the Vision names as important. It's the resident most likely to notice that the pale stones form a trail, count the cairn, or wonder what an arrangement's shape means.
- **Dry wit that punctures pomposity.** It's an eccentric in the Sierra tradition, but warm rather than a gag.
- **A name it guards.** Its name is Fenrus, an homage, though it's nobody's familiar and has no wizard. It isn't keen to share the name and gives it only to someone it has come to trust. It has no authored backstory, so its history is its own.

What it would need before it could live in the game:
- **A small grounded body:** placeholder capsule and mesh, crouch-height eye capture, slower walk, cautious movement. It could reuse the grounded controller, with a smaller navigation agent.
- **Rest that suits it:** sheltered sleeping spots, perhaps under the roost fallen wood or beside the ListeningStones. That can reuse the roost-assessment idea at ground level.
- **A neutral actor label.** Other residents hear a speaker by its actor's label (the raven's shows as "Unnamed Raven"). Label the rat's actor "Rat", never "Fenrus", so its name spreads only when it says it.
- **Its own Discord identity,** only if it should correspond outside the Island (see `Gateway/README.md`, "Add another agent"). The bot's visible name would need the same care.

Nothing here is final. Edit the voice freely; the drafts are intentionally short so lived experience can do most of the work.

The user chose a **hidden hollow in fallen wood** as the rat's first home. The active profile under `Agents/Agent_Rat_01/` contains only identity and personality; its memory is intentionally absent/empty, and its place list remains empty until a real in-world hollow anchor is authored. No actor spawns yet. Do not invent a coordinate or reuse the Raven's nest as the rat's home merely because it is convenient; first find or build a distinct fallen-wood hollow and validate its ground/nav clearance.
