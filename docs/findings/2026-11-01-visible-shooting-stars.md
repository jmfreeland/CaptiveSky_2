# Make the rare shooting-star report visible — 2026-11-01

## Change

`AIslandDayNight::DescribeTime` could tell residents that a shooting star was
crossing the sky, but the sky renderer only had stationary star cards. The
procedural starfield now includes one preallocated, hidden meteor card. During
the first 16% of an eligible clear, dark ten-minute Island-time slot, the card
moves across the sky on a deterministic path seeded by Island day and slot;
then it hides again. Resident time descriptions only mention the meteor during
that same brief visible crossing.

The event reuses the existing star mesh and vertex-color material. It adds only
four vertices and two triangles, has no collision, and does not add actors,
textures, Niagara systems, random state, or model requests. The existing rare
slot probability, weather and moon gates, resident pacing, and star populations
are unchanged.

## Validation and limits

`IslandDayNightTests.cpp` now checks the extra transient mesh section, its
four-vertex footprint, a deterministic clear-night event, visible motion
window, and matching resident narration before and after the crossing. The UE
5.8.3 scratch Editor target compiled and linked successfully in five UBT actions
(38.54 seconds), including both modified source files. The focused automation
compiled but was not executed: the known NullRHI pre-initialization crash
remains unresolved, and the open D: editor does not load this worktree's module.

The live machine currently reports only 258 MiB free on the 12,282 MiB GPU at
75 °C. GPU process names are inaccessible to this workspace, and system RAM
queries are denied. No rendered run is safe or claimed. The card is negligible
in geometry size, but its appearance and winding still need an editor capture
after memory headroom recovers; the known NullRHI startup failure also prevents
blindly repeating a headless test.
