# Periwinkle nocturnal grazing — 2026-10-18

Tideglass periwinkles now keep their existing small, reversible graze path
active through the day, while its activity clock advances more readily at
night. The Island's existing day/night curve supplies a smooth transition at
dawn and dusk: midday activity is 45% of the nighttime rate, with twilight
blending between them. Tide exposure and short-lived disturbance still pause
or slow grazing as before. If no Island clock is available, the old full-rate
behavior is preserved.

This changes only the progression rate of the existing 16 cm-bounded crawl. It
adds no actors, collision, saved state, model requests, or map/content edits.
The regression checks daytime activity, stronger nighttime activity, a
mid-twilight value, wrapped hours, and a safe fallback for invalid clock input.

## Validation and limits

The UE 5.8.3 `CaptiveSky_2Editor` target built and linked successfully from the
isolated `Saved/CompileScratch/Codex_WindArchVisitor_20261018` project with
`-NoHotReloadFromIDE` and three actions. UnrealBuildTool compiled both
`IslandTidepoolPeriwinkle.cpp` and `IslandTidepoolPeriwinkleTests.cpp`.

The automation was not launched. Earlier focused Tideglass automation runs
using the same NullRHI startup path terminated at project startup (`SecondsSinceStart=0`)
before creating their requested test log; the project-startup finding advises
not to repeat that unchanged launch. There is no Unreal Editor process running
for a visual check, so the in-world difference remains unverified. Once the
startup path is repaired, run
`CaptiveSky2.Agent.TidepoolPeriwinkleNocturnalGrazing` and compare a close
Tideglass view near midday and at night.
