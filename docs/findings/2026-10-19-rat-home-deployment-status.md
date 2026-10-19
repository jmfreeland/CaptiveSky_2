# RatHollow is not yet a live Island home (2026-10-19)

## Current-state check

- The local `WorldState/Island.json` has an empty `wildlife_homes` array.
- `Agents/Agent_Rat_01/places.json` has an empty `places` array.
- `UIslandRatSubsystem::FindRatHomeLocation` therefore cannot select a
  canonical Rat home. `SpawnRat` retains its existing Inn-side fallback when
  no eligible home record exists; this source check does not prove that the
  Rat pawn actually spawned in the current editor session.
- The RatHollow inspection path now names the Rat's fallen-wood hollow and its
  automation fixture checks `CanInteract` plus nearest-visible-target
  discovery, as well as read-only state. Both the interaction and test source
  compiled and linked in an isolated UE 5.8 scratch target (five actions); the
  automation was not run.
- The project already has a RatHollow visual candidate, but prior site notes
  say its shelter, concealment, ground clearance, and capsule/nav traversal
  have not all passed together at a specific Island coordinate.

## Decision

Keep both persistent home arrays unchanged. The user chose a hidden
fallen-wood hollow as the Rat's home concept, but that is not approval of an
exact Island location. Do not convert a candidate coordinate into canonical
state or resident place-memory until the placed asset and approach are
visually reviewed and physically traversed by the Rat.

## Next step

In a disposable Island review, evaluate a sheltered candidate from both human
approach and Rat-eye views, verify the imported hollow entrance and ground
contact, then test NavMesh projection and capsule traversal. Only after that
passes should a reviewed coordinate be recorded in both world state and the
Rat's places, with any map placement explicitly saved and verified.

At the latest check, the user reports the editor open and responsive, while a
fresh Unreal-MCP request to `127.0.0.1:8000` fails to connect. The most recent
desktop capture showed an asset-reopen prompt over the editor; no click, map
change, or persistent-state write was made.
