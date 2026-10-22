# Dragonfly response to a quiet visitor

## Change

Tideglass dragonflies now briefly flutter away when a player-controlled pawn is
within 220 cm, within 220 cm vertically, and moving no faster than 110 cm/s.
The response is temporary (1.6 seconds). A per-pawn presence latch requires the
visitor to leave a 480 cm ring before another response can occur; an additional
eight-second cooldown bounds responses across different visitors. Checks are
staggered at 0.45-second intervals and do not issue model requests or write
world state. The existing direct observation interaction remains available.

The `CaptiveSky2.Agent.NightEcology` regression test covers the brief scatter,
direction away from the visitor, no repeat while the visitor lingers, and
rearming after leaving and returning.

## Verification

- UE 5.8.3 `CaptiveSky_2Editor` scratch build succeeded on 2026-10-19. UHT
  completed and all 65 build actions succeeded, including compilation of
  `IslandTideglassDragonfly.cpp` and `IslandEcologyTests.cpp` and linking the
  scratch module. The main editor's loaded DLL was not rebuilt or replaced.
- The focused automation was not verified. Two headless `UnrealEditor-Cmd`
  attempts remained responsive and consumed CPU but did not create either the
  requested log or the project's default log. The exact command-line process
  launched for this check was stopped; no crash report appeared. Do not treat
  this as a passing test or as an editor hang.
- The interactive editor and saved Island map were left untouched. Unreal MCP
  remained unreachable at `127.0.0.1:8000` during this work.

## Next step

Diagnose why this scratch project's command-line editor stops before opening
its log, then run `CaptiveSky2.Agent.NightEcology` and inspect the response in
bounded real-time play. Keep the Island map unchanged unless a later visual
review explicitly requires an edit.

## Focused live-editor automation (2026-10-22)

In the open UE editor, these non-world-fixture tests all passed (3/3, 0.108 s
total): `CaptiveSky2.Agent.Ecology.CommonFrogDragonflyAttention`,
`CaptiveSky2.Agent.Ecology.DragonflyFrogResponse`, and
`CaptiveSky2.Agent.Ecology.DragonflyListeningStonesResponse`. The direct frog
response test had no warnings. The other two logged `UWorld::DestroyActor:
World has no context!` warnings while cleaning up `IslandDewActor`,
`IslandDrift`, `IslandGradeActor`, `IslandRainbowActor`, and `IslandTrailMarks`
from transient `Untitled` worlds; they still had no test failures. No PIE or
model requests were started. This resolves the earlier missing unit-test
evidence, but not the separate in-world visual response check.
