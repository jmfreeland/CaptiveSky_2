# Give the Island's lunar clock a visible disc — 2026-11-01

## Change

The Island already modeled a persistent lunar cycle, moonrise/set, cloud-hidden
moon, and phase-sensitive moonlight, but no visible moon appeared in the sky.
The shared procedural star mesh now includes a small circular moon disc at the
direction of the actual moon light. Its vertex color dims with lunar
illumination, cloud transmission, and the authored moon-light intensity. It is
hidden in daylight, below the horizon, at new moon, when moonlight is disabled,
or when thick cloud hides the moon.

The disc is a 48-triangle fan at the existing 20 km sky shell. It adds 49
vertices and no actor, texture, material, Niagara system, collision, world-state
write, or model request. This first pass conveys phase through brightness; it
does not yet shape a crescent or add surface texture.

## Validation and limits

`IslandDayNightTests.cpp` now checks the extra mesh section, its vertex/index
budget, and visibility at noon, new moon, clear full moon, and under thick
cloud. The UE 5.8.3 scratch Editor target compiled and linked successfully in
five UBT actions (14.50 seconds), including both modified C++ files. The focused
automation compiled but was not executed: the known NullRHI pre-initialization
crash remains unresolved, and the open D: editor does not load this worktree's
module.

The latest measured GPU sample before this change left only 258 MiB free on the
12,282 MiB device; no rendered session was started. Check the disc's brightness,
apparent size, cloud interaction, and horizon placement in a matched night
capture when both GPU and physical-RAM headroom meet the play gate. A true
waxing/waning crescent shape remains a later art pass.
