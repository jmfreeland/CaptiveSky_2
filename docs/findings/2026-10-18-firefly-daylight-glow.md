# Firefly daylight glow — 2026-10-18

The firefly's natural pulse now shares the Island's smooth dusk/dawn curve. At
midday its glow holds at 16% of the previous daytime value; through twilight
it rises gradually to the full night pulse. Rain, moonlight, the ordinary
flash rhythm, and brief visitor/chime accents continue to modulate that value.
If the actor has no Island clock, the prior full glow behavior is preserved.

This is presentation-only. It adds no actors, lights, persistent state, model
requests, or map/content edits. The ecology regression covers dim daytime,
full nighttime, intermediate dusk, wrapped hours, and invalid-clock fallback.

## Validation and limits

The UE 5.8.3 `CaptiveSky_2Editor` target built and linked successfully in the
content-bearing isolated project
`Saved/CompileScratch/Codex_VisitorResonance_20261017` using
`-NoHotReloadFromIDE` and 13 actions. UnrealBuildTool compiled
`IslandFirefly.cpp` and `IslandEcologyTests.cpp`.

The bounded NullRHI `NightEcology` automation was not launched. Preflight
continued to find the user's `UnrealEditor` PID 22688 and Windows reported it
as not responding; it was left untouched. No daylight-to-night visual
comparison is available yet. Run
`CaptiveSky2.Agent.NightEcology` when the editor is responsive or closed, then
compare the same firefly at midday, dusk, and full night in the ordinary
Tideglass view.
