# Visitor reading of worn paths

The island already keeps a bounded, persistent footfall ledger. Repeated paths wear ground-cover instances away, weather slowly lets grass return, rain can leave temporary prints, and residents can notice those traces. Visitors had no equivalent way to read this small record of shared movement.

When no ordinary landmark or wild creature is close enough to answer, a visitor standing over a recorded path or cluster of wet prints now gets an `E`/`Interact` hint to read the nearby tracks. Interacting reports the trail subsystem's existing local description. The path ledger remains unchanged; no new save, model request, or autonomous simulation is added. Nearby landmark interactions retain priority.

## Verification

- The UE 5.8.3 `CaptiveSky_2Editor` Development build succeeded (9 actions, including both changed translation units).
- `CaptiveSky2.Agent.IslandTrailVisitor` passed under NullRHI with `CaptiveSkyMaxModelRequests=0`; the synthetic visitor checks the hint, factual caption, and unchanged footfall count.
- The synthetic test checks logic, not the visual readability of the trail material in the live Island.

## Next

During the next bounded Game/PIE visual pass, confirm that the worn path or wet prints can actually be seen from the visitor camera before deciding whether their material or size needs tuning.
