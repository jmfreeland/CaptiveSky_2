# Island seasons

The persistent Island day now also places residents in a simple temperate
calendar: spring, summer, autumn and winter, each 90 Island days long. The
calendar repeats every 360 days. The existing lunar phase remains on its own
29.53059-day cycle; the two clocks do not reset or force each other into
alignment.

`AIslandDayNight::DescribeCalendar` includes the season in the existing
`Island.SkyReport`, so residents can situate observations and memories in the
time of year. The existing qualitative air-warmth and wind/rain-chill cues now
also use a small smooth annual offset: neutral at the spring/autumn boundaries,
warmest near midsummer, and coolest near midwinter. Cloud moderates the seasonal
and daily swings together. This is descriptive felt warmth, not a measured
temperature or weather forecast; it does not yet change daylight, plant
appearance, or animal behavior. Those seasonal ecology changes should be
introduced as separate, evidence-grounded steps rather than implied by the
calendar alone.

The focused `IslandDayNight` regression covers the season boundaries, the first
day of the next year, the annual warmth extrema, cloud damping, and the resulting
wind/chill description. Validation should also check that a saved Island day
continues through the boundary without resetting the independent lunar phase;
later visual ecology work can build on the same persistent day number.
