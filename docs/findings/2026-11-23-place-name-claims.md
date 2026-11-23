# Resident place-name claims — 2026-11-23

## First implementation slice

Residents may optionally offer a personal name for a place they have already
recorded in their own `places.json`. The claim is accepted only on a delivered,
nearby resident-to-resident line, only when the spoken line contains the chosen
name, and only when the speaker's remembered-place list contains the exact
anchor ID. Names are short, plain labels. Invalid or unspoken claims are ignored.

The speaker and listener receive separate append-only conversation memories.
Each record retains the stable place ID, claimant or source resident, source
conversation ID, and turn number. The listener's memory says who used the name;
knowledge is not written into other residents' files. The place ID and map label
are never changed, and competing personal names can coexist. This adds no model
call and does not make naming a required action.

The resident's own decision prompt includes their remembered anchor IDs, so an
opening line or response can refer to a place they know without relying on the
speaker's memory or inventing an identifier. These IDs are not added to player
or external-correspondence context.

`Scripts/Inspect-PlaceNameClaims.py` reads resident `memory.jsonl` files and
lists only records tagged `place-name`. It skips blank lines, rejects malformed
nonblank records, and can emit JSON for deterministic inspection. Pass
`--agents-dir` with the isolated `Agents` folder to inspect a separate data
root. Running it against the checked-in
agent data currently reports no place-name claims, as expected before the new
runtime path is built and played.

## Limits and verification

This is a memory-and-transmission slice, not community adoption or a map marker.
The system does not choose a canonical name, count popularity, rename an actor,
or claim that the listener agrees. A listener can later use the attributed name
in their own speech, but this first slice does not infer endorsement from that
alone.

The source change has not been rebuilt or exercised in Unreal yet. No automated
tests were added or run. After the shared editor is available, rebuild and
inspect the runtime memory files after one resident proposes a name and another
resident receives it; verify that an unrelated resident has no claim record.
