# Firefly daytime rest — 2026-10-18

The existing fireflies now keep their broad, slow flight loop close to their
home point during bright hours, then gradually open into their normal wider
night wandering through twilight. The scale is bounded from 22% by day to full
range at night. It modulates horizontal drift and the gentle vertical bob;
home height, weather response, glow, chime response, actor count, collision,
and per-tick obstacle sweeps remain unchanged. Worlds without an Island clock
retain the previous full wander range.

The existing `NightEcology` regression now checks daytime range, full night
range, twilight interpolation, wrapped hours, and safe invalid-time fallback.
This is local presentation only: no resident/model requests, saved state, new
actors, or Content/map changes.

## Validation and limits

The UE 5.8.3 `CaptiveSky_2Editor` target built and linked successfully in the
content-bearing isolated project
`Saved/CompileScratch/Codex_VisitorResonance_20261017` using
`-NoHotReloadFromIDE` and 18 actions. UnrealBuildTool compiled both
`IslandFirefly.cpp` and `IslandEcologyTests.cpp`, as well as the latest
periwinkle sources.

The bounded NullRHI `NightEcology` run was not launched: immediately before
launch, Windows reported a live `UnrealEditor` process (PID 22688) as not
responding. It was left untouched. The previous known-good test command uses
MCP disabled, thinking disabled, zero model requests, an isolated data root,
and a 120-second cap; retry when the editor is responsive or closed. No
in-world daytime-to-night visual comparison has been made.
