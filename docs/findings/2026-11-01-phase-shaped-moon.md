# Phase-shaped lunar disc — 2026-11-01

## Change

The visible moon previously used one uniformly bright circular disc, so its
shape disagreed with the Island's waxing/waning phase description. The shared
procedural sky mesh now spans the illuminated portion of the lunar disc only.
For each of 49 horizontal rows, the lit chord follows the phase's projected
sun direction: waxing phases illuminate from the right; waning phases from the
left; quarter moons expose half the disc; and full moon fills the circle. The
unlit region is omitted from the mesh, preserving stars instead of painting a
dark circle over them.

The mesh has 98 vertices and 96 triangles, reuses the existing vertex-color
sky material and sky shell, and adds no actor, texture, material, Niagara
system, collision, world-state write, or model request. Moon-light brightness
continues to follow illumination, while the rendered lit surface keeps a
consistent albedo so a crescent is thinner rather than uniformly dimmer.

## Validation and limits

`IslandDayNightTests.cpp` now checks the fixed geometry budget, two illuminated
limbs near full moon, and the one-sided lit arc of a clear, risen waxing
crescent. This is geometry evidence, not a rendered appearance check. The
change still needs a matched new/quarter/full/last-quarter night capture to
inspect orientation, smoothness, apparent size and whether the source vertex-
color material renders the exposed lit mesh as intended.

The actual D: editor is currently stopped at a startup prompt: the project's
`CaptiveSky.uproject` enables the `Aura` plugin, but no project `Plugins`
directory exists and a read-only search found no `Aura.uplugin` under either
the installed UE 5.8 engine's plugin tree or `D:\Projects - Athena\Unreal`.
The prompt offers to disable Aura or close the launch. No choice has been made;
do not change the project descriptor or accept plugin disabling without the
user's direction. The existing low GPU headroom and 30-FPS/memory safeguards
remain in force.
