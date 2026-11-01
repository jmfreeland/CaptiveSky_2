# Area-uniform radial sampling for woodland layers — 2026-11-01

## Change

Woodland candidates were given a radius uniformly between each grove layer's
inner and outer limits. That does not distribute points uniformly across the
ground area: annular bands have more area at larger radii, so uniform-radius
sampling over-represents the grove center per square metre.

`AIslandWeather::SampleUniformAnnulusRadius` now maps one stable `[0, 1]`
sample through `sqrt(lerp(inner², outer², sample))`. The grove-center offsets
and the mature tree, sapling, shrub, rhododendron, and coppice layers all use
this mapping. The sample still consumes one value from the existing seeded
stream. Instance budgets, species selection, clearances, cull ranges, and the
overlapping habitat footprints are unchanged.

The focused broadleaf regression checks the analytic area median, both annulus
endpoints, clamping, and non-finite input handling.

## Validation

The UE 5.8.3 isolated Editor target compiled and linked successfully after the
helper and regression were added (38 build actions). The actual D: editor has
the separate project-level Nanite Foliage setting enabled, but it does not
load this worktree's module. The math automation compiled but has not been
executed in Unreal.

No rendered comparison is claimed. At the latest sample the whole device had
2,391 MiB free, below the project's 4 GiB rendered-session gate. The known
worktree NullRHI startup failure also remains unresolved. When an editor with
this module can safely run, compare the same seeded Island route and report
species/layer counts, visible clumping, frame-time p95, and whole-device VRAM;
the math test alone does not prove the new distribution looks more natural.
