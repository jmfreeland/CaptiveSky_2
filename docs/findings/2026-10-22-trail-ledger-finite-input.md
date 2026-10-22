# Reject invalid footfalls before saving trail memory (2026-10-22)

`FIslandTrailLedger::AddStep` now rejects positions or ground normals with non-finite components, and rejects finite XY positions whose 1.5 m cell coordinates cannot fit in the ledger's `FIntPoint` key. Rejected footfalls leave existing path memory unchanged. This prevents invalid movement transforms from introducing NaNs, infinities, or overflowing cell indices into persistent trail JSON.

Validation used the open UE 5.8 editor without starting PIE or changing the Island map:

- Live Coding compiled successfully.
- `CaptiveSky2.Agent.IslandTrail` passed (1/1, 0 failures, 0 warnings), including NaN position, infinite normal, and out-of-range cell regressions.

This is a persistence-integrity safeguard; it does not alter trail appearance or claim that paths have been visually validated in the current world.
