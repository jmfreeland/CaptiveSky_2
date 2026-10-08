# Aster shore-to-landmark navigation probe

## Result

On 2026-10-08, an isolated UE 5.8.3 Game run rebuilt dynamic navigation after
releasing the startup `AsyncLoadLock`. Recast completed in 1.31 seconds. The
storm-wrack point `(-44480.88, 103520.22, 1069.24)` projected to walkable
navigation at Z `1085.733`; the resulting route to `ListeningStones` was
complete and 585 m long.

The same bounded run then physically moved Aster from his normal runtime start
to `ListeningStones`. The controller found a complete 15 m route (6 points),
started movement, and reported arrival after 3.0 simulated seconds and 1,327 cm.
The final location was `(-101562.83, 101598.78, 2688.79)`. The isolated Game
session ended after 83.5 real seconds with zero model requests.

## Scope and limits

This is evidence that the runtime can build the needed navigation, that the
storm-wrack point connects to the landmark on that rebuilt navmesh, and that
Aster can physically walk to the landmark from his normal start. It does **not**
prove a physical walk from the storm-wrack point: `Island.MoveProbe` started
Aster at the map's normal runtime start. It also does not prove that Aster
interacts with or discovers anything at the Listening Stones.

The first harness attempt called `Island.MoveProbe` through `UWorld::Exec`; the
nav probe logged no movement-command result. The scratch-only harness now finds
the registered `IConsoleCommand`, executes it with the probe world and arguments,
and records whether dispatch succeeded. This avoided a game-source change.

## Reproduction

The successful run used the ignored scratch project at
`Saved/NavBoundsTest/Project/CaptiveSky_2.uproject`, loaded `/Game/Maps/Island`,
and invoked:

```text
Island.NavBuildProbe -44480.88 103520.22 1069.24 60 MoveAsterToStones
```

It used `-CaptiveSkyDisableAgentThinking`,
`-CaptiveSkyMaxRealtimeSeconds=120`, `-CaptiveSkyMaxModelRequests=0`, and an
isolated world-data root. The scratch editor target build succeeded with
`-NoHotReloadFromIDE`; only `IslandNavProbe.cpp` and the scratch module link were
needed.

Detailed runtime log: `Saved/NavBoundsTest/Project/Saved/Logs/AsterDynamicNavMoveProbe2.log`.
