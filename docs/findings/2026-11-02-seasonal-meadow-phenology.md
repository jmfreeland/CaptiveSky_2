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

NullRHI establishes the calendar and caption behavior, not a visual claim.
Rendered flower silhouettes and colors, pollinator choices during these added
windows, and frame-time impact still need a memory-gated review once at least
4 GiB of GPU memory is free. Do not raise the flower instance budget as part of
that review.
