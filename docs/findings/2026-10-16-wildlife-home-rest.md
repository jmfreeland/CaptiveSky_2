# Residents can rest at their own recorded wildlife home — 2026-10-16

## Behavior

The resident's situation now names its assigned wildlife home as an optional,
exact `move_to` target only when the home is within 25 m and visible. After
arrival, the resident may choose sleep at `WildlifeHome_<id>`; rest still
requires the body to be grounded and within the existing 250 cm arrival
tolerance. A wildlife-home target must exist in world state, its presentation
actor must carry the matching home tags, and its occupant must match the
resident's own agent ID. Another resident cannot sleep at that assigned home.

Rest records `LastUsedUtc` and queues a place-specific memory that is written
only if the sleep interval completes; waking early cancels it. The description
explicitly treats this as evidence of where the resident rested, not proof of
weather protection, safety, comfort, or recovery. Sleep remains optional, does
not teleport, and does not restore health. Unnamed sleep at the resident's own
home is recognized after arrival as well.

## Validation

UE 5.8.3 built the current shared `Source` junction through the isolated
`Saved/CompileScratch/Claude_Props` project, using its own `Binaries` so the
main project DLL held by the user's editor was not replaced. Compile and link
both succeeded.

`CaptiveSky2.Agent.IslandInnRest` passed in that scratch project under NullRHI
with `-CaptiveSkyMaxModelRequests=0` and Python disabled. The fixture verifies
home perception, exact target naming, visibility limits, no distant sleep or
teleport, ownership rejection, last-used persistence, cautious memory wording,
early-wake cancellation, and completed-rest memory. The fixture uses an
isolated world-state path and a unique automation memory identity. No model
request appears in the run log; the fixture itself does not request an
autonomous decision.

The test did not modify the saved Island or approve a Rat home location. The
existing North Woodland candidate still needs a real visual, Rat-eye and
human-approach review plus a physical capsule/navigation check before it is
recorded in the Rat's places file.
