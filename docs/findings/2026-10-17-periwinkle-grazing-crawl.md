# Bounded periwinkle grazing crawl

The Tideglass periwinkle previously tucked its eye stalks and aperture in
response to exposure, rain, crabs, ravens, and a close visitor, but otherwise
remained anchored to its spawn point. It now makes a very slow closed graze
path while extended. The path completes over two minutes of active time, is
offset deterministically by the snail's existing location-derived phase, and
stays within 16 cm of its home point. Its active clock slows in partial shelter
and pauses while fully tucked.

Each movement sample traces down to a nearby, walkable surface and accepts only
ground within 24 cm of the spawn height. If no such surface is found, the snail
holds its current position. This keeps the visual-only actor on its rock instead
of walking across a ledge or following an unrelated high surface. The movement
is transient: it adds no collision, navigation influence, saved state, or model
requests. The existing 0.2-second actor tick bounds surface checks to one per
snail per tick.

The new pure automation coverage checks that the path starts and returns home,
stays inside its displacement bound over a full cycle, advances when extended,
slows under partial shelter, pauses when fully tucked, and rejects invalid
timing input.

## Validation and limits

The UE 5.8.3 `CaptiveSky_2Editor` target built and linked successfully from the
isolated `Saved/CompileScratch/Codex_BasinRipple_20261018` project with
`-NoHotReloadFromIDE` and two actions. The build compiled both
`IslandTidepoolPeriwinkle.cpp` and `IslandTidepoolPeriwinkleTests.cpp`.

A focused NullRHI automation launch was attempted with agent thinking disabled
and the model-request limit set to zero. It exited without creating the
requested automation log or reporting a test result. Therefore the regression
is compile-validated but not automation-verified, and no visual/gameplay claim
is made. The main editor and Island map were left untouched.

Next: once the editor/startup path is usable, run
`CaptiveSky2.Agent.TidepoolPeriwinkleGrazingPath`, then inspect a close Tideglass
view at high and low tide to confirm that the crawl reads as quiet grazing and
that rock-edge traces hold safely.
