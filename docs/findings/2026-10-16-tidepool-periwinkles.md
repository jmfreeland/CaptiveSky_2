# Tideglass periwinkle life — 2026-10-16

Three small periwinkles now occupy ground-traced positions around the Tideglass
shoreline. Each uses a compact primitive fallback body with a procedural shell
growth spiral and one of three muted natural shell colors. Their components are
non-colliding, do not affect navigation, and use a slow antenna sway. The
bounded population persists through the Island's day/night cycle and is cleaned
up with the rest of transient Tideglass ecology; no map, Content asset, or
persistent world state is changed.

## Verification

- The UE 5.8.3 `CaptiveSky_2Editor` target built and linked successfully in the
  isolated `Saved/CompileScratch/Claude_Props` project after the new actor,
  weather integration, and `IslandEcologyTests.cpp` compiled.
- The test adds checks for the three-member cap, shoreline placement, shell
  geometry, distinct palettes, no-collision behavior, idempotent refresh, and
  persistence through day and night.
- The attempted NullRHI `CaptiveSky2.Agent.NightEcology` run exited with code
  zero but did not create its requested log or a test result. Treat it as an
  unsuccessful startup, not a passing test. The retained crash context was not
  updated by this attempt, so its cause remains unknown; see
  [`Unreal startup crash triage`](2026-10-13-unreal-startup-triage.md).
- No real-RHI capture exists yet. The procedural silhouette, palette, ground
  placement, and close-range readability remain unreviewed in-game.

## Next step

After project startup can be observed reliably, run `CaptiveSky2.Agent.NightEcology`
under NullRHI with thinking and model requests disabled. Then make one short,
fixed Tideglass shoreline capture to review scale and material response before
deciding whether the fallback is good enough or should be replaced with a
proper snail mesh.
