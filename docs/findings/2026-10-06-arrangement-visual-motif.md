# Shared visual motifs for resident-made stone art (2026-10-06)

The arrangement lineage used to exist in canonical data and resident-facing descriptions, but not in the stones themselves. A source work now owns a stable `MotifSeed`; when a resident names a previously seen source and changes the form, the descendant inherits that seed while retaining its own layout seed and different overall form.

`AIslandArrangement` materializes the motif as three small pale stones in a deterministic local pattern. These marks have no collision and do not affect navigation, placement, or interaction rules. A first sighting of a descendant still reveals only its public form and lineage; titles and intent remain private. Existing records without `motif_seed` use their own layout seed, preserving their shape and requiring no migration.

Validation performed:

- The UE 5.8.3 `CaptiveSky_2Editor` target built successfully on 2026-10-06.
- `CaptiveSky2.Agent.IslandArrangement` and `CaptiveSky2.Agent.IslandArrangementInspection` both passed. The first verifies a new mark, exact shared local transforms across a different-form descendant, and persistence through world-state reload. Log: `Saved/Logs/Codex_ArrangementMotif_20261006_02.log`.
- The fixture makes no model requests and writes only to its isolated `Saved/Automation` state.

The geometry and deterministic identity can be tested headlessly, but that does not establish that the mark is legible or attractive from normal play cameras. Review a live close-up and a standard journey view when the editor is available. Resident choice to make, notice, or transmit a motif also remains unverified; the mark is a cue, not a forced cultural behavior.
