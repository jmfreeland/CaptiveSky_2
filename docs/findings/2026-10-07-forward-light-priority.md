# Day/night forward-light selection (2026-10-07)

## Evidence and change

A recent dusk Game capture showed Unreal's screen warning that multiple
directional lights compete for forward shading, translucent water, and
volumetric fog. `AIslandDayNight` owns the changing Sun, Moon, and Starlight
intensities, but did not update their `ForwardShadingPriority` values.

After each lighting update, the brightest of those three sources now receives
the unique top priority. The remaining lights receive distinct lower values,
so a zero-intensity or secondary light cannot tie the active source. Priorities
are changed only when their value needs to change. This changes no authored map
data or saved world state.

## Validation

The UE 5.8.3 `CaptiveSky_2Editor` Development target built successfully. The
focused `CaptiveSky2.Agent.DayNight` and `CaptiveSky2.Agent.DayNightPersistence`
automations passed with NullRHI; the lighting test checks unique priorities and
that the brightest light wins at noon, dusk, new moon, and full moon. Log:
[`Codex_ForwardLightPriority_20261007.log`](../../Saved/Logs/Codex_ForwardLightPriority_20261007.log).

A real-RHI `CaptiveSky2.Visual.Viewpoints` capture at 17:18 also passed, with no
forward-priority warning in its log. The captured frame is an editor
SceneCapture with transient rock preview, not the Game viewport; it cannot
confirm whether Unreal's on-screen warning is gone in normal play. Screenshot:
[`04_WindArchOverlook.png`](../../Saved/Viewpoints/2026-10-07_112155_h17.3/04_WindArchOverlook.png).

A bounded 90-second, provider-free spectator/Game launch was attempted with an
isolated world-state root. It stayed in `TurnkeySupport` before the Island
loaded, so the runtime cap never became active; the exact process started for
this probe was stopped. Log:
[`Codex_ForwardLightPriorityGame_20261007.log`](../../Saved/Logs/Codex_ForwardLightPriorityGame_20261007.log).

**Startup follow-up:** the accompanying `AutoSDKInfo.txt` reports
`ValidatePlatforms` succeeded in 0.21 seconds. A low-CPU `dotnet.exe` process
started at the same time as that validation remained after the Game parent was
stopped; that exact child was then stopped too. This correlation does not prove
that the child caused the Turnkey wait, and the process did not reach map load.

### Bounded dusk Game verification (2026-10-07)

A standalone Island Game launch used the normal saved viewpoint route at a
17:18 starting hour, with agent thinking and Python disabled, isolated world
state, a four-minute realtime cap, and a one-request ceiling. It reached
world-ready in 15 seconds, captured ten frames including the 17:18 opening
frame and the Wind Arch at 18:59, then exited cleanly after 240.2 seconds with
zero model requests. Log:
[`Codex_ForwardLightDusk_20261007.log`](../../Saved/Logs/Codex_ForwardLightDusk_20261007.log).

The earlier directional-light warning was not reproduced: its message is
absent from this Game log, and no warning banner is visible in either the
17:18 opening frame or the later Wind Arch frame. This verifies the warning is
not present in this bounded Game route after the priority fix; it does not
prove every renderer, map, or lighting configuration is warning-free. Frames:
[`001_Shore_Approach.png`](../../Playtests/Codex_ForwardLightDusk_20261007/Screenshots/001_Shore_Approach.png)
and
[`008_Wind_Arch_Overlook.png`](../../Playtests/Codex_ForwardLightDusk_20261007/Screenshots/008_Wind_Arch_Overlook.png).
