# Landmark navigation audit and WindArch access (2026-10-09)

## Finding

Aster could not use the WindArch from the saved Island start. The navmesh reported
complete routes, but the grounded approach code projected each stand-off point with
only one narrow query. That missed nearby walkable floor around the elevated WindArch
marker. After that was corrected, Aster could reach the Arch, but sometimes stopped
outside the four-metre inspection range because the planned goal used nearly all of
the available range and path-following could stop short of it.

## Change

Grounded landmark approaches now use the existing grounded-target projection helper,
which checks nearby floor before an elevated fallback. The controller tests five
stand-off radii (150, 200, 250, 325, and 375 cm), capsule-sweeps complete routes, and
keeps the planned body position 100 cm inside the shared interaction range. The added
development-only `Island.LandmarkNavAudit [DelaySeconds]` command reports these
projection, route, and capsule-clearance checks for each tagged movement landmark;
it never moves an actor or interacts with the world. Supplying a delay from 5 to 90
seconds releases the runtime async-load nav lock, requests a navigation build, and
runs the audit after the delay. The build and audit affect only that test session.

## Evidence

UE 5.8 scratch editor build succeeded. `CaptiveSky2.Agent.BlockedGroundMoveApproach`
passed.

In the bounded WindArch movement probe, Aster used one capsule-clear staging move to
get around the large rock near his start, then found six clear landmark approaches.
He travelled 1,228 cm in 3.0 simulated seconds, arrived within inspectable range, and
performed the normal WindArch interaction: a short-lived local gust and three
illuminated motes. The effects were transient; no lasting weather state changed. The
run ended after 44.7 real seconds with zero model requests (120-second watchdog,
zero-request cap).

The final non-interactive direct-route audit covered five movement-eligible landmarks:

| Landmark | Complete candidate routes from Aster's saved start | Capsule-clear | Notes |
| --- | ---: | ---: | --- |
| ListeningStones | 19 | 0 | Direct routes meet the large `SM_Rock`; the staged Aster interaction was separately verified in the previous milestone. |
| TideglassPool | 20 | 15 | Best clear route was 872 cm. |
| WindArch | 12 | 0 | Direct routes from the saved start were blocked; the staged physical move and interaction succeeded. |
| Inn marker | 14 | 0 | Direct routes meet the same rock; this is a marker audit, not a test of the InnCounter or InnHearth routes. |
| RainBasin | 20 | 4 | Best clear route was 581 cm. |

These are direct routes from one start position, not reachability verdicts. A zero
direct-route clearance can still be recoverable by the existing staging behavior.
The WindArch run demonstrates that for this landmark; Inn and RainBasin should be
physically tested before changing their collision or movement rules. Several blocked
sweeps hit a grass mesh actor, so its pawn collision deserves inspection before any
vegetation collision is altered.

## Logs

- `../../Saved/Playtests/Codex_LandmarkNavAudit_20261009/LandmarkNavAudit_Final.log`
- `../../Saved/Playtests/Codex_LandmarkNavAudit_20261009/AsterWindArchMove_Final.log`
- `../../Saved/Playtests/Codex_LandmarkNavAudit_20261009/BlockedGroundMoveApproach.log`

## Next

Check whether the grass actor that blocks some direct sweeps actually blocks the
walker's Pawn channel, then run bounded InnCounter and RainBasin moves. Preserve the
staging fallback unless those probes show a real failure after recovery.
