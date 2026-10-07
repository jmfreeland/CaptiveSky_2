# Headless ecology regression check (2026-10-07)

While the interactive editor and Unreal MCP endpoint were unavailable, the UE
5.8.3 editor target reported success and up to date (0 build actions). The
existing editor objects postdate the current shared edits in
`IslandWeather.h` and `IslandViewpointCapture.cpp`, so the following automation
ran against the current compiled snapshot rather than requiring a source edit:

- `CaptiveSky2.Agent.NightEcology` — passed in 26.9 seconds.
- `CaptiveSky2.Agent.IslandWeather` — passed in 24.6 seconds.

Both used `UnrealEditor-Cmd`, `-NullRHI`, an in-memory DDC, and the automation
queue exit condition. They did not start a gameplay session or make live model
requests. Logs: [`Codex_NightEcology_20261007.log`](../../Saved/Logs/Codex_NightEcology_20261007.log)
and [`Codex_IslandWeather_20261007.log`](../../Saved/Logs/Codex_IslandWeather_20261007.log).

This validates deterministic ecology and weather regressions only. It does not
validate the rendered vegetation composition, actual PIE traversal, frame-time
distribution, or the user's 30-FPS minimum. Keep those as open gates; once UE
and MCP are available, compare the same fixed gameplay-scale view and a separate
bounded PIE traversal against the recorded baselines. No source or authored
Content asset was changed for this check.
