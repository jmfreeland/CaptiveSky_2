# Periwinkle response to heavy rain — 2026-10-16

Tideglass periwinkles already respond to the exposed ebb and to close, quiet
observation, but unlike the fireflies they did not read the Island's weather.
They now sample the existing deterministic `AIslandWeather` rain signal at the
same 0.2-second tick interval and draw their eye stalks, aperture, and gentle
body motion in slightly during a heavy shower. The effect is bounded to at most
0.42 rest, so it layers under the stronger ebb/observation tuck and never
removes the snail or changes its home. In dry weather the response is zero.

This is presentation-only ecology: no movement, memory, world-state, or model
request is added. If no weather actor is present, the periwinkle keeps its
previous behavior.

## Validation

- UE 5.8.3 editor target built in the isolated
  `Saved/CompileScratch/Claude_Props` project; UHT passed and the changed
  periwinkle implementation and `IslandEcologyTests.cpp` compiled and linked.
- `CaptiveSky2.Agent.NightEcology` passed under `UnrealEditor-Cmd -NullRHI`
  with exit code 0. Its fixture compares the same periwinkle under a seeded
  heavy shower and dry weather, checks that the stalks tuck slightly and then
  return to full extension, and tests the pure response's bounds, monotonicity,
  and non-finite input handling.
- Log: [`Codex_NightEcology_RainResponse_20261016.log`](../../Saved/CompileScratch/Claude_Props/Saved/Logs/Codex_NightEcology_RainResponse_20261016.log).

This does not yet show whether the small visual cue reads clearly in ordinary
play. Inspect it at Tideglass during a naturally occurring shower once the
Island editor/game path is available; keep the standard playtime and model-call
caps in place.
