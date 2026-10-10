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
- The UE 5.8.3 real-D3D12 `CaptiveSky2.Visual.RavenWingMotion` test also passed
  with agent thinking disabled, zero model requests, and isolated world data.
  Its transient editor-world scene capture rendered the encounter without
  saving the map or world state.

Evidence: [automation log](../../Saved/Logs/Codex_RavenDragonflyAttention_20261010.log),
[real-RHI visual log](../../Saved/Logs/Codex_RavenDragonflyVisual_20261010_final.log),
[staged encounter frame](../../Saved/Viewpoints/RavenWingMotion_/20261010_104406/11_RavenDragonflyGlance.png),
[controller source](../../Source/CaptiveSky_2/Agent/RavenAgentAIController.cpp),
[regression test](../../Source/CaptiveSky_2/Agent/Tests/RavenPerchTests.cpp).

The widened scene capture now keeps both actors in frame and confirms the
attention pose renders, but it is not a hero image: the pair sits off-center,
the Raven's pale placeholder surface reads as a blockout, and the ground is
still brown and thinly planted. This staged editor capture does not establish
natural encounter frequency or normal gameplay readability. Reframe it on a
better planted pocket after the current vegetation/material pass, then recheck
in bounded Game/PIE play.
