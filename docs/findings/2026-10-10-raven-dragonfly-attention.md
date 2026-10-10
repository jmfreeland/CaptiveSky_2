# Raven attention to a nearby dragonfly (2026-10-10)

When the Raven is settled on the ground or a perch, its existing nearby
wildlife attention can now include a visible `AIslandTideglassDragonfly` within
10 m and 2.5 m of vertical separation. The cue points the existing brief head
turn at the insect's body; it does not move the Raven, startle or redirect the
dragonfly, write memory/world state, or make a model request. A dragonfly is
acknowledged once while it remains inside the shared 14 m nearby-group radius;
leaving that radius rearms the cue. Other closer wildlife and higher-priority
attention remain in the existing selection path.

## Verification

- The UE 5.8.3 `CaptiveSky_2Editor` target built and linked successfully.
- `CaptiveSky2.Agent.RavenPerch` passed with the Island map loaded, NullRHI,
  Python disabled, isolated world data, and `-CaptiveSkyMaxModelRequests=0`.
  Added assertions cover noticing a nearby dragonfly, no Raven movement,
  one-shot behavior, and rearming after the insect leaves and returns.
- UBT AutoSDK validation returned `0` in this approved run. The recurring
  desktop `dotnet.exe` exception did not recur here; this successful launch
  cannot explain or disprove the user's separate popups.

Evidence: [automation log](../../Saved/Logs/Codex_RavenDragonflyAttention_20261010.log),
[controller source](../../Source/CaptiveSky_2/Agent/RavenAgentAIController.cpp),
[regression test](../../Source/CaptiveSky_2/Agent/Tests/RavenPerchTests.cpp).

This is deterministic logic coverage, not a rendered wildlife encounter.
Recheck head-turn readability and ordinary-distance framing in a bounded
real-RHI Game capture after the current vegetation/material pass.
