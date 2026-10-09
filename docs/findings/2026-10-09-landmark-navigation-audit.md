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
keeps the planned body position 100 cm inside the shared interaction range. Staging
recovery preserves the requested target tag even for movement-only targets the
interaction helper does not recognize. It samples capsule-clear reachable waypoints
within 25 m and prefers forward progress toward the requested target, rather than
repeatedly choosing the nearest short hop. The added development-only
`Island.LandmarkNavAudit [DelaySeconds]` command reports these
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
direct-route clearance can still be recoverable through staging. The WindArch run
demonstrates this for an interaction landmark. Aster also reached RainBasin in 494 cm
and 1.0 simulated second, with no interaction requested. The InnCounter probe first
exposed a recovery bug: staging retried by resolving the target through an interaction
helper, which does not recognize movement-only counters. After preserving the original
tag, the probe exposed a second issue: the old nearest-waypoint heuristic repeatedly
chose short hops inside the same cluttered region. The target-aware selector chose a
capsule-clear stage 1,412 cm closer to InnCounter; from there, five of sixteen candidate
approaches were clear, and Aster arrived after 1,998 cm in 4.0 simulated seconds.
Both bounded runs used a 120-second watchdog and zero model requests.

The physical sweeps identified both the large `SM_Rock` and map-placed grass actors as
Pawn-channel blockers. Grass actors reported `affects-nav=1` and `pawn-response=2`
(`ECR_Block`). This is useful evidence, not yet a reason to weaken vegetation collision:
the read-only level audit identified them as `Roost_Undergrowth_*`, around the
`Roost_West_NaturalLedge`. The rock is the ledge itself; the nearby Tripo props are the
intentional East/West Inn lantern posts. These authored components use `BlockAll`,
`QueryAndPhysics`, and affect navigation, so no map or global foliage collision was
changed. A reusable read-only `Scripts/Inspect-IslandPawnBlockers.py` audit records
their actor labels, bounds, mesh paths, and collision responses without saving the map.

Aster's ListeningStones move provides a second target check: all 19 direct approaches
from the saved start were capsule-blocked, but the target-aware staging move found 16
clear approaches afterward. Aster reached the stones after 1,079 cm in 3.0 simulated
seconds, again with zero model requests.

## Logs

- `../../Saved/Playtests/Codex_LandmarkNavAudit_20261009/LandmarkNavAudit_Final.log`
- `../../Saved/Playtests/Codex_LandmarkNavAudit_20261009/AsterWindArchMove_Final.log`
- `../../Saved/Playtests/Codex_LandmarkNavAudit_20261009/AsterRainBasinMove.log`
- `../../Saved/Playtests/Codex_LandmarkNavAudit_20261009/AsterInnCounterMove_AfterFix4.log`
- `../../Saved/Playtests/Codex_LandmarkNavAudit_20261009/AsterListeningStonesMove.log`
- `../../Saved/Playtests/Codex_LandmarkNavAudit_20261009/PawnBlockerAudit_Methods.log`
- `../../Saved/Playtests/Codex_LandmarkNavAudit_20261009/BlockedGroundMoveApproach.log`

## Next

Preserve the authored Roost collision and use the staging behavior where it is needed.
These successful routes cover Aster's saved start only, not every resident or every
approach. Next, exercise a raven roost return and a second grounded start before making
any wider claim about landmark reachability.
