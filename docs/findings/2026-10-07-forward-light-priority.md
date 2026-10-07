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

The source-level selection and editor-render test are verified; actual Game
selection and the original dusk screen warning remain unverified. Recheck a
matched dusk Game sequence when a normal Island Game/PIE launch is available.
