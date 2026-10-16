# Tideglass periwinkle ground placement — 2026-10-16

## Finding

`CaptiveSky2.Agent.NightEcology` initially found no periwinkles. The first of
the three deterministic shoreline traces landed outside the test's intentionally
partial ground fixture. `RefreshNightEcology()` stopped at that miss, so it
never attempted the other two valid homes. This was a real placement weakness:
a collision gap at one sample could suppress the entire quiet shoreline
population.

## Change

Each periwinkle now keeps its own deterministic 120-degree shoreline sector and
checks up to nine points across that sector (the center, then alternating
15-degree steps through +/-60 degrees) before giving up on that individual
home. A missed ray no longer prevents later homes from being considered. The
search is bounded and does not change weather, save state, or population size.

The automation's temporary periwinkle inspection also moved a real spawned
snail to the pool center but left it there. Once the missing population was
fixed, this leaked test fixture affected later visitor target selection. The
test now restores both its original location and home after the inspection.

## Validation

- UE 5.8.3 `CaptiveSky_2Editor` built through the isolated
  `Saved/CompileScratch/Claude_Props` project; only `IslandWeather.cpp` and,
  on the follow-up build, `IslandEcologyTests.cpp` compiled before the scratch
  module relinked.
- `CaptiveSky2.Agent.NightEcology` passed under `UnrealEditor-Cmd -NullRHI`
  (exit code 0), including three bounded daytime periwinkles, repeat-refresh
  population stability, and persistence through dusk/night/day. The visitor
  target-selection assertions also passed with the test snail restored to its
  habitat.
- Log: [`Codex_NightEcology_Verified_20261016.log`](../../Saved/CompileScratch/Claude_Props/Saved/Logs/Codex_NightEcology_Verified_20261016.log).

This is isolated automation evidence, not a visual or saved-Island gameplay
review. Inspect the actual Tideglass shore in the editor once the project can be
opened reliably; confirm that the sampled arc lands on natural-looking ground
and does not crowd the pool edge.
