# Ground-cover community field warp — 2026-11-02

## Change

Ground-cover species selection now samples the existing 42 m continuous field
at a smoothly displaced world position. Two separately seeded 24 m fields
offset that sample by at most 9 m in each horizontal axis. The intent is to
make species-community edges meander without adding foliage instances, mesh
types, timers, or per-frame work. Wet-edge and exposed-landmark habitat fields
remain sampled in their original world positions, and the candidate/instance
budgets are unchanged.

## Validation

The UE 5.8.3 `CaptiveSky_2` Game target built successfully after the change.
This confirms the runtime source compiles and links. The editor remains open,
so the updated `IslandEcology` automation was not run against the new Editor
module. A fixed-seed automation assertion now checks that the warped edge
contains neighboring species while a nearby pair remains coherent, but both
compilation and execution of that Editor test remain pending: UBT refused both
the main and isolated scratch Editor builds because Live Coding is active. No
rendered comparison was captured; the change should be judged in a matched
Island view once GPU headroom permits. Keep this as a reversible composition
experiment until that review.
