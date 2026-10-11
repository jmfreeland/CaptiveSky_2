# Canonical wildlife-home record (2026-10-11)

The persistent world-state store now has a renderer-independent record shape
for wildlife homes: stable place ID, species and occupant, entrance anchor and
orientation, bounded shelter quality/state, last-used and last-disturbed UTC
times, and a small named-supply cache. Storage caps are 32 homes, eight cache
slots per home, and 20 units per slot. Invalid IDs, non-finite transforms,
duplicate cache items, and out-of-range quantities are rejected. Updating or
removing a record saves atomically and rolls back the in-memory change if the
save fails.

The new `wildlife_homes` JSON field is additive. Older version-1 world-state
files without it still load, and the automation fixture checks that an
existing nest survives a wildlife-home write/read/removal round trip. The
record is currently data-only: it does not materialize an actor, authorize a
site, select a home coordinate, or update `Agents/Agent_Rat_01/places.json`.
Fenrus' hollow remains provisional until the disposable-scene understory,
ground contact, movement, and navigation checks pass.

The project built successfully in an isolated scratch project's own
`Binaries/` and `Intermediate/` directories while the user editor remained
open. The focused `CaptiveSky2.Agent.WildlifeHomeRecord` automation passed in
`UnrealEditor-Cmd -NullRHI`; the existing editor DLL was not relinked or
modified. Build and test logs are under the ignored
`Saved/CompileScratch/Codex_WildlifeHomeRecord_20261011/Saved/Logs/` directory.
