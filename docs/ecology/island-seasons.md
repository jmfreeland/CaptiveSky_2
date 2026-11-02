# Island seasons

The persistent Island day now also places residents in a simple temperate
calendar: spring, summer, autumn and winter, each 90 Island days long. The
calendar repeats every 360 days. The existing lunar phase remains on its own
29.53059-day cycle; the two clocks do not reset or force each other into
alignment.

Daylight now changes smoothly through that calendar: about eight hours around
midwinter, twelve at the spring and autumn equinox points, and sixteen around
midsummer. Sunrise, sunset, solar elevation, golden-hour fill, and residents'
"daylight left" descriptions all use the same seasonal arc. No fixed weather
or temperature forecast is implied by this change.

`AIslandDayNight::DescribeCalendar` includes the season in the existing
`Island.SkyReport`, so residents can situate observations and memories in the
time of year. The qualitative air-warmth and wind/rain-chill cues also use a
small smooth annual offset: neutral at the spring/autumn equinox points,
warmest near midsummer, and coolest near midwinter. Cloud moderates the
seasonal and daily swings together. This is descriptive felt warmth, not a
measured temperature or weather forecast. Firefly and tidepool-periwinkle
activity, minnow rest, frog night activity, lichen glow, and the day/night
routines for shore and woodland wildlife now follow the seasonally changing
sunrise and sunset. The minnow transition keeps its former equinox timing but
eases at the new winter/summer horizons. Woodland bluebells and Tideglass marsh
marigolds now bloom in spring; yellow-flag iris spans spring and summer. Those
specialist instances go dormant by component visibility outside their authored
seasons. The eight generic Fab flower forms now use overlapping visual bloom
windows across spring, summer, and autumn, all reaching dormancy at the winter
boundary. Their component and instance position seed a small stagger so a
whole patch does not open or close together. These are aesthetic schedules for
unnamed meshes, not claims about the exact plant species. Visitors can now hear
when nearby generic wildflowers are beginning to bloom or fully open. This
changes no instance budget or collision state and updates only on ecology
refresh, not every frame. Rendered readability, pollinator selection across
the new windows, seasonal regrowth, color change, and broader tree phenology
remain future work.

The focused `IslandDayNight` regression covers the season boundaries, annual
photoperiod and warmth extrema, day/night intervals, cloud damping, and the
resulting resident daylight and wind/chill descriptions. The
`IslandWeather.SeasonalFlowers` automation covers specialist and generic
bloom windows, spring/autumn boundaries, year wrap, and compatibility behavior
for unknown indices. A 2026-11-02 UE 5.8.3 scratch build and focused `-NullRHI`
automations also pass; see [seasonal meadow phenology](../findings/2026-11-02-seasonal-meadow-phenology.md).
The production flower query now has a focused regression confirming spring and
autumn bloom eligibility and winter dormancy. Rendered readability and actual
dragonfly attraction across the new windows remain unverified. Validation
should also check that a saved Island day continues through a season boundary
without resetting the independent lunar phase; later visual ecology work can
build on the same persistent day number.
