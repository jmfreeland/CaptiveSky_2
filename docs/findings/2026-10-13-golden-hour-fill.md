# Golden-hour ambient fill at Wind Arch (2026-10-13)

The fixed 17:00 Wind Arch overlook showed warm low-angle sunlight, but the
Listening Stone, inn wall and approach collapsed into very dark shadow. The
sun already had a warm horizon tint, so this pass did not change its color,
direction or intensity. Instead, the existing cool shadowless twilight fill
begins earlier and the skylight rises by up to 70% near sunset. The scale fades
back to exactly 1.0 as full night arrives; the moon, night skylight floor,
resident schedule and request safeguards are unchanged.

The UE 5.8.3 editor target built successfully, including the revised
`IslandDayNightTests.cpp`. The same no-world-state, real-RHI 17:00 capture
passed before and after. Sampled mean luminance (0–255) from identical image
regions changed as follows:

| Region | Before | After |
| --- | ---: | ---: |
| Central rock | 34.59 | 43.63 |
| Inn wall | 31.42 | 48.61 |
| Wind Arch pier | 77.52 | 104.86 |
| Deep ground shadow | 2.47 | 2.56 |

The fill makes the rock, wall and landmark pier easier to read without
changing the direct warm-sun silhouette. The ground shadow itself remains
nearly black; increasing global fill further would flatten the rest of the
scene, and this capture's ground/material presentation deserves a separate
material review.

- Before: [`04_WindArchOverlook.png`](../../Saved/Viewpoints/2026-10-13_181935_h17.0/04_WindArchOverlook.png)
- After: [`04_WindArchOverlook.png`](../../Saved/Viewpoints/2026-10-13_182501_h17.0/04_WindArchOverlook.png)
- Before log: [`Codex_WindArchGoldenHourAudit_20261013.log`](../../Saved/Logs/Codex_WindArchGoldenHourAudit_20261013.log)
- After log: [`Codex_WindArchGoldenHourFill_Viewpoints_20261013.log`](../../Saved/Logs/Codex_WindArchGoldenHourFill_Viewpoints_20261013.log)

The focused `CaptiveSky2.Agent.DayNight` automation compiled but could not
execute: the project-opening path crashed at `SecondsSinceStart=0` with the
recurring access violation (`PCallStackHash`
`F405610C114D120D4B6F5ABA901EC469AC9FAA26`) before the requested project log
or test began. The unit assertions cover sunset lift and exact full-night
baseline restoration, but remain runtime-unverified until that startup fault
is diagnosed. No map asset, world-state file or request budget changed.
