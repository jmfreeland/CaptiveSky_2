# Tidepool crab response to player presence — 2026-10-17

## Change

The shore crab now notices a player-controlled pawn within 240 cm and 220 cm
vertically, then briefly scurries away. It reacts once per approach, rearms only
after the player leaves a 500 cm horizontal radius, and has an eight-second
response cooldown. Detection shares the existing 0.35-second ecology scan. It
does not call an LLM, send network traffic, alter saved state, or change
collision. The behavior is covered by assertions for approach direction,
lingering, cooldown, and re-entry in `CaptiveSky2.Agent.NightEcology`.

## Validation

UE 5.8.3 UnrealHeaderTool completed and both changed translation units,
`IslandTidepoolCrab.cpp` and `IslandEcologyTests.cpp`, compiled successfully in
the main project build. Linking then failed because the running
`UnrealEditor.exe` held `Binaries/Win64/UnrealEditor-CaptiveSky_2.dll` open;
the editor was left untouched at the user's request. Therefore the new
automation assertions have compiled but have not run, and the behavior has not
been observed in play. Do not terminate the editor or overwrite the loaded
module to complete this validation. Once the editor is safely closed, rebuild
and run `CaptiveSky2.Agent.NightEcology`, then observe the crab approaching and
leaving the shore in play.
