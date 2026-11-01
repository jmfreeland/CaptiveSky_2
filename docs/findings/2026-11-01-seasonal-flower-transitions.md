# Seasonal flower transitions (2026-11-01)

The named woodland bluebell, Tideglass marsh marigold, and yellow-flag iris
patches no longer switch between fully absent and fully grown at a season
boundary. A deterministic smoothstep now stages instance scale across the first
and last Island day of each authored bloom window. A low-frequency spatial
field offsets each instance by up to 0.6 Island days, so a whole habitat patch
does not open or fade in lockstep. The progression follows the fractional
Island clock and is sampled by the existing 30-second ecology refresh; it adds
no actor, instance, collision, navigation, or per-frame update.

The existing ground-cover sway still starts from immutable baseline transforms
and now composes with each seasonal scale. Flower-seeking wildlife ignores
instances whose blossoms are effectively closed, while the existing broad
season description for residents remains intact. The eight general meadow
flower forms remain year-round because their species identities do not yet
have authored calendars.

The isolated UE 5.8.3 Editor target compiled and linked successfully, including
the updated `IslandWeatherSeasonTests.cpp` coverage for spring opening/fade,
the longer iris season, calendar wrap, year-round forms, and non-finite input.
The automation test itself was not run: the known NullRHI startup crash remains
an unresolved behavior-test gate. No rendered session was started because
whole-device GPU headroom was 3,384 MiB, below the 4 GiB screen. In-game pivot
readability, pollinator selection during the transitions, and frame-time cost
remain unverified. The next check is a matched short rendered observation once
the memory gate is met; stop if system or graphics memory pressure recurs.
