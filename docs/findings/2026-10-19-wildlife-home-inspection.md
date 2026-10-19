# Read-only wildlife-home inspection (2026-10-19)

Materialized wildlife homes now participate in the shared inspection path as
`WildlifeHome` targets. Inspection reads the canonical shelter record and
describes whether it is exposed, provisional, settled, or disturbed. It does
not expose the occupant agent ID or hidden cache contents, and it does not
write state or disturb the shelter. The existing home actor remains a
collisionless, presentation-only materialization.

## Verification

- `IslandInteractionUtility.cpp` and `IslandNestTests.cpp` compiled and linked
  successfully in the isolated UE 5.8 scratch project
  `Saved/CompileScratch/Codex_CurrentSourceBuild_20261018` (five UBT actions).
- The `WildlifeHomeRecord` automation now checks target discovery, the visitor
  description, omission of private/hidden details, and unchanged canonical
  shelter state and cache quantity.
- The automation was **not run**. A project commandlet previously crashed
  during pre-init without discovering its requested test; the open editor was
  left untouched. Compilation is not runtime or in-world evidence.

## Next check

When the editor bridge/startup path is available, run
`CaptiveSky2.Agent.WildlifeHomeRecord` in an isolated world-state root and then
inspect the RatHollow through the normal resident interaction path. Verify
that its visual bounds are discoverable at the intended range and that
inspection never changes the saved home record. Separately, keep the main
Island placement gate: a test-world result alone does not authorize moving the
RatHollow or adding clutter around its approach.
