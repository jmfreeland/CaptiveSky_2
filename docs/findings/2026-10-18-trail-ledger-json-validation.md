# Validate persisted footpath cells before loading (2026-10-18)

The IslandTrail ledger is a small piece of persistent environmental memory: it
records where visitors repeatedly walk, lets those routes weather back toward
grass, and gives residents a read-only way to notice old paths. Its JSON parser
previously cast cell coordinates and step counts directly from arbitrary
numbers. A malformed but parseable row could therefore overflow an integer
conversion, place wear at a coordinate that did not belong to its cell, or
provide impossible ground-normal components.

`FIslandTrailLedger::FromJson` now validates the complete candidate ledger
before replacing live state. It rejects non-finite values, fractional schema
versions/cell keys/step counts/decay days, out-of-range integer and world-cell
coordinates, impossible ground normals, duplicate non-empty cells, and more
rows than the ledger's 6,000-cell capacity. Zero-step rows are ignored. The
loader continues to fail closed: an unreadable saved file disables further
storage for the session instead of overwriting the original.

Regression coverage was added for fractional schema/cell values, an extreme
finite world coordinate, impossible normals, and an out-of-range decay day;
the existing round-trip, valid path weathering, and refusal-preserves-ledger
checks remain in `CaptiveSky2.Agent.IslandTrail`.

The UE 5.8.3 editor target compiled and linked successfully in the isolated
`Codex_FireflyPulseClean_20261018` scratch project. Both changed translation
units were compiled; the project startup/automation run was not repeated
because the known Core null-access failure occurs before project tests begin
(see [startup triage](2026-10-13-unreal-startup-triage.md)). Thus behavior
assertions are present but have not been executed against Unreal in this pass.
No saved map, Content asset, trail ledger, or resident/model-request behavior
was changed.

Next verification: once the existing project-open crash is resolved or the
runtime environment meaningfully changes, run only
`CaptiveSky2.Agent.IslandTrail` with thinking and model requests disabled, then
check a short zero-request visitor walk for a normal worn path and post-rain
prints.
