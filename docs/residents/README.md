# Proposed residents

Drafts of future residents are kept outside `Agents/` so the game and gateway never load them by accident. To bring one to life, copy its identity and personality into `Agents/`, give it a body, and let it start with an empty memory.

Current status: `Agent_Rat_01` (the rat) has an active identity/personality profile and spawns in Island Game/PIE when its local body asset is available. The chosen hidden fallen-wood hollow is a home concept, not yet a validated canonical place record, so runtime spawning can still fall back to a navigable point near the Inn. The active profile is in `Agents/Agent_Rat_01/`; keep lived memory, relationships, places, and personality-evolution state local to each data root rather than copying them between checkouts. `Agent_Innkeeper_01` is also active: `IslandInnkeeperSubsystem` spawns its configured body in the Island play world, and its identity/personality live under `Agents/Agent_Innkeeper_01/`; the older innkeeper draft wording below no longer describes runtime status.

## Agent_Rat_01: the rat (initial identity draft 2026-09-27; active profile 2026-10-10)

This answers the README's *Quest for Glory* note about an unusually intelligent rat. A pointer for when the reference is settled: in *Quest for Glory*, Erasmus is the wizard and his talking rat familiar is Fenrus. This rat is deliberately neither. It's an original being with its own home on the Island and no master, in keeping with the Vision: residents are participants rather than quest dispensers, and Sierra-inspired characters should be original beings rather than reproductions.

What it's meant to add:
- **A different scale.** It lives low to the ground, relies on shelter and caution, and notices what is under and between things. The raven sees from above; Aster sees at eye level.
- **Patterns, mathematics and humour,** three things the Vision names as important. It's the resident most likely to notice that the pale stones form a trail, count the cairn, or wonder what an arrangement's shape means.
- **Dry wit that punctures pomposity.** It's an eccentric in the Sierra tradition, but warm rather than a gag.
- **A name it guards.** Its name is Fenrus, an homage, though it's nobody's familiar and has no wizard. It isn't keen to share the name and gives it only to someone it has come to trust. It has no authored backstory, so its history is its own.

Runtime status and remaining considerations:
- **Body and movement:** the Rat has a grounded runtime controller and placeholder body path. Use the small scale, cautious movement, and low viewpoint as characterization, not as an excuse to invent perception or abilities the body does not have.
- **Home:** the user chose a hidden hollow in fallen wood. That is a home concept, not a validated Island anchor; until a specific shelter passes concealment and navigation review, the runtime spawner may use its documented Inn-side fallback.
- **Name:** keep the actor's public label "Rat", never "Fenrus"; the Rat decides whether and when to share its own name.
- **Discord identity:** optional only if it should correspond outside the Island (see `Gateway/README.md`, "Add another agent").

The personality is intentionally light so lived experience can do most of the work. Edit the voice carefully; changes should not overwrite or fabricate the Rat's runtime memories.

The profile files in this source checkout are static identity/personality text only. Lived memory, relationships, places, and personality-evolution state belong to their originating runtime data root; do not copy them between checkouts. The runtime spawner prefers a matching Rat wildlife-home record (settled shelters first) and retains the established Inn-side fallback if no record exists or its entrance has no nearby navigation. Do not invent a coordinate or reuse the Raven's nest as the Rat's home merely because it is convenient; first find or build a distinct fallen-wood hollow and validate its ground/nav clearance. See the [runtime spawn handoff](../findings/2026-10-11-rat-home-runtime-spawn.md).
