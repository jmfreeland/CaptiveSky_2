# Rat spawn facing follows the selected RatHollow (2026-10-19)

`FIslandWildlifeHomeRecord` stores the RatHollow entrance position and facing,
but `UIslandRatSubsystem` previously selected only the position and spawned the
Rat at zero yaw. Home selection now returns the chosen, finite, normalized
entrance rotation as well; the Rat spawn uses its yaw. If the home has no
navigable point and the subsystem falls back to the Inn, facing resets to the
existing neutral orientation.

The home-selection regression covers the selected record's facing and verifies
that a failed search leaves caller-provided location and facing defaults
untouched. This is deliberately limited to spawn orientation: it does not move
the canonical entrance, create navigation, or add a home to the saved Island.

## Verification

- UE 5.8 `CaptiveSky_2Editor` built successfully in
  `Saved/CompileScratch/Codex_CurrentSourceBuild_20261018`. UBT reported six
  actions and success; the Rat subsystem and home-selection test object files
  are present in the scratch target.
- The focused `CaptiveSky2.Agent.RatHomeSelection` automation was not run. The
  project's commandlet startup failure remains unresolved; no additional
  Unreal process was launched and the live editor/map were not modified.

## Next check

Once the isolated automation route is available, run the home-selection test,
then verify the spawned Rat's yaw at the approved fallen-wood hollow in a
bounded, zero-request play session. Do not infer that the RatHollow is already
placed or navigable from this source/build check.
