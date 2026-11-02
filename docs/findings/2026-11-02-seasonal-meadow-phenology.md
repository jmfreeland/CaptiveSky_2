# Seasonal meadow-flower phenology — 2026-11-02

## Change

The eight generic Fab meadow-flower HISM components previously stayed at full
scale all year. They now follow overlapping three-day bloom transitions across
spring, summer, and autumn; all eight are dormant by the first winter day.
Component-specific stable spatial noise offsets each instance by up to 0.6
Island days so neighboring blooms do not transition in lockstep. Their existing
transform baseline, sway, HISM counts, collision, and navigation settings are
unchanged. The existing ecology refresh updates their visibility and transforms;
there is no additional actor, timer, or per-frame scan. The schedules are
explicitly aesthetic because the generic imported mesh forms are not
botanically identified.

Nearby visitor observations now recognize `IslandMeadowFlowerA` through `H` as
seasonal wildflowers and can report that they are beginning to bloom or in
bloom. Closed/dormant components are hidden by the same existing update path.

## Validation

UE 5.8.3 `CaptiveSky_2Editor` Development built and linked in the isolated
`Saved/CS58` project. Focused NullRHI automations
`CaptiveSky2.Agent.IslandWeather.SeasonalFlowers` and
`CaptiveSky2.Agent.NearbyVegetationPerception` both passed, including spring
opening, summer/autumn overlap, winter dormancy, smooth transitions, the annual
wrap, and a visitor's nearby wildflower-bloom caption. The final log is
`Saved/Logs/Codex_SeasonalMeadowPhenology_Verified_20261102.log`. The run
disabled agent thinking and model requests and used a dedicated playtest data
root.

## Pollinator query follow-up

The `CaptiveSky2.Agent.MeadowFlowerSpecies` NullRHI automation now also drives
the production seasonal visibility/scaling update and the exact
`FindNearestMeadowFlower` query used by dragonflies. It confirms that the
spring form can be found in spring, is absent from the query in autumn, a
long-season meadow form can be found in autumn, and no generic bloom is
available in winter. Log: `Saved/Logs/Codex_SeasonalPollinator_20261102.log`.
This verifies query eligibility, not a rendered dragonfly flight or visitor
readability; those still need a memory-gated visual review.

NullRHI establishes the calendar and caption behavior, not a visual claim.
Rendered flower silhouettes and colors, actual dragonfly attraction during
these added windows, and frame-time impact still need a memory-gated review
once at least 4 GiB of GPU memory is free. Do not raise the flower instance
budget as part of that review.

## Dragonfly flight integration regression

`CaptiveSky2.Agent.Ecology.DragonflySeasonalFlowerVisit` now joins the
production seasonal visibility update and nearest-bloom query to a dragonfly's
temporary flower hover and flight response. Its synthetic, asset-free fixture
checks that a spring bluebell is queryable and draws a dragonfly into a brief
hover, that the dragonfly bends its flight toward the hover station, and that
the same bloom is unavailable in summer. It does not add actors or alter the
Island's scatter, tick cadence, state, or request budget.

The UE 5.8.3 `CaptiveSky_2Editor` Development target compiled and linked
successfully (17 build actions, 209 seconds), including the new fixture and
both production ecology translation units. The automation was not launched:
fresh Editor-Cmd/Game startup is still failing before test discovery in this
environment, and repeating the same launch would not provide new evidence.
Thus the integration check is compile-verified, not runtime-passed; a rendered
pollinator visit also remains gated on at least 4 GiB free GPU memory.
