# Night ecology player fixture — 2026-10-17

The first plugin-disabled `CaptiveSky2.Agent.NightEcology` run reported four
shore-crab failures: no startle, no scurry direction, no cooldown, and no re-arm.
The fixture's `APlayerController` successfully returned the spawned pawn from
`GetPawn()`, but the pawn's `IsPlayerControlled()` was false in this isolated
world. Production code that enumerates every `APawn` and relies on that flag
therefore skipped the controller-owned visitor.

`AIslandTidepoolCrab::CheckForNearbyPlayerPresence` now enumerates actual
`APlayerController` actors and reads each controller's pawn. This makes the
source of player control explicit and works for remote as well as local player
controllers; it still excludes AI-controlled wildlife. The test verifies the
controller owns the visitor, then exercises one-shot response, direction,
cooldown, linger suppression, and re-arm after leaving the wider ring.

The same run caught a regression from the firefly-corridor improvement: all
three night fireflies were put in the Listening Stones chime range, while the
existing ecology invariant expects exactly one landmark listener. The spawner
has been corrected to anchor only its first firefly near the Stones and leave
the other two at their habitat.

## Verification

- UE 5.8.3 `CaptiveSky_2Editor` scratch target linked successfully (35 actions,
  65.84 s).
- `CaptiveSky2.Agent.NightEcology` passed under `-NullRHI`, with model requests
  disabled and world-state writes disabled, in the isolated scratch project,
  both with `ModelContextProtocol` disabled and with it enabled.
- `CaptiveSky2.Agent.FireflyCorridorPlacement` passed again with the corrected
  single-anchor helper, then passed once more with the MCP plugin enabled.
- These headless tests verify automation behavior and project startup only; they
  do not verify real-RHI visuals or gameplay feel.
