# Resident place-name claims — 2026-11-23

## First implementation slice

Residents may optionally offer a personal name for a place they have already
recorded in their own `places.json`. The claim is accepted only on a delivered,
nearby resident-to-resident line, only when the spoken line contains the chosen
name as a complete phrase rather than a substring inside a longer word, and only
when the speaker's remembered-place list contains the exact anchor ID. Matching
is case-insensitive. Names are short, plain labels. Invalid or unspoken claims
are ignored.

The listener must accept the utterance into its conversation queue before the
speech, social records, or promised side effects are published. A final line
that closes a reserved conversation is delivered without waiting for another
reply. Rejected lines leave no place-name memories or Chronicle entry.

The speaker and listener receive separate append-only conversation memories.
Each record retains the stable place ID, claimant or source resident, source
conversation ID, and turn number. The listener's memory says who used the name;
knowledge is not written into other residents' files. The place ID and map label
are never changed, and competing personal names can coexist. This adds no model
call and does not make naming a required action.

The delivered exchange also writes a `place_name_claim` event to the world
Chronicle, including the stable place ID, listener, conversation ID, and turn.
`Scripts/Build-Chronicle.py` renders it as an attributed personal name. The
Chronicle is for observers; it does not reveal the event to other residents.

Both participants' memory tags now retain the exact `place-label` separately
from the prose. `Scripts/Inspect-PlaceNameClaims.py` includes that label as a
field and in its readable output, while keeping each resident's own knowledge
distinct; it does not count mentions as community adoption. For older records
without the tag, the inspector falls back to the quoted label in the memory text.

The resident's own decision prompt includes their remembered anchor IDs, so an
opening line or response can refer to a place they know without relying on the
speaker's memory or inventing an identifier. These IDs are not added to player
or external-correspondence context.

Preflight of the checked-in resident profiles found that both Aster and the
Innkeeper already know the same `Cairn` anchor ID and coordinates in their
separate `places.json` files. This is a concrete shared target for the first
runtime exchange; it does not establish that either resident prefers a new name
or that another resident knows one.

`Scripts/Inspect-PlaceNameClaims.py` reads resident `memory.jsonl` files and
lists only records tagged `place-name`. It skips blank lines, rejects malformed
nonblank records, and can emit JSON for deterministic inspection. Pass
`--agents-dir` with the isolated `Agents` folder to inspect a separate data
root. Its `--knowledge` view groups distinct residents by stable place ID and
label, retaining whether each entry is a claim or learned-from record; it does
not count mentions as agreement or adoption. The checked-in agent data currently
has no place-name claims, as expected before the new runtime path is built and
played.

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
