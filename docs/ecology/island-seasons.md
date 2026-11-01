# Island seasons

The persistent Island day now also places residents in a simple temperate
calendar: spring, summer, autumn and winter, each 90 Island days long. The
calendar repeats every 360 days. The existing lunar phase remains on its own
29.53059-day cycle; the two clocks do not reset or force each other into
alignment.

Daylight now changes smoothly through that calendar: about eight hours around
midwinter, twelve at the spring and autumn equinox points, and sixteen around
midsummer. Sunrise, sunset, solar elevation, golden-hour fill, and residents'
"daylight left" descriptions all use the same seasonal arc. No fixed weather,
temperature forecast, or seasonal plant growth is implied by this change.

`AIslandDayNight::DescribeCalendar` includes the season in the existing
`Island.SkyReport`, so residents can situate observations and memories in the
time of year. The qualitative air-warmth and wind/rain-chill cues also use a
small smooth annual offset: neutral at the spring/autumn equinox points,
warmest near midsummer, and coolest near midwinter. Cloud moderates the
seasonal and daily swings together. This is descriptive felt warmth, not a
measured temperature or weather forecast. Firefly and tidepool-periwinkle
activity, frog night activity, and lichen glow now follow the seasonally
changing night interval. Seasonal plant growth and appearance remain future
work.

The focused `IslandDayNight` regression covers the season boundaries, annual
photoperiod and warmth extrema, day/night intervals, cloud damping, and the
resulting resident daylight and wind/chill descriptions. Validation should also
check that a saved Island day continues through the boundary without resetting
the independent lunar phase; later visual ecology work can build on the same
persistent day number.
