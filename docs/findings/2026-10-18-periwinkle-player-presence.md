# Periwinkle response to a nearby player — 2026-10-18

Tideglass periwinkles now notice a player pawn approaching within 2.4 m and
220 cm vertically. A close approach briefly reuses the existing reversible
shell-tuck response; lingering does not repeat it, and the snail rearms only
after the player leaves a 5.2 m ring. This is sampled on the existing 0.4 s
presence cadence. It does not move or persist the animal, add collision, make
an LLM request, or change the resident interaction API.

The focused UE 5.8.3 `CaptiveSky_2Editor` build completed successfully with
`-NoLink`; UnrealHeaderTool and both `IslandTidepoolPeriwinkle.cpp` and
`IslandEcologyTests.cpp` compiled. The tests cover first approach, lingering,
rearm after leaving, and rejection of a high-overhead pass. Automation and
visual behavior were not run because the project-opening startup crash remains
unresolved. The user's unresponsive editor PID 22688 was left running.

Next: once project startup is stable, run `CaptiveSky2.Agent.NightEcology` and
watch a grounded player approach and leave the Tideglass edge. Confirm the
subtle tuck is visible without the snails appearing to flee or repeatedly
startle during ordinary shoreline play.
