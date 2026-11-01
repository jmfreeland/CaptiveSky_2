# Ocean low tide and day/night validation (2026-11-01)

## Ocean low-water correction

The UE 5.8 Water plugin's `UWaterBodyOceanComponent::SetHeightOffset` did not
retain the Island's negative low-tide offset in the isolated automation world:
after the authored 4.5 cm offset and a spring high, the low-water setter read
back as exactly 0 cm rather than the expected -9.483 cm. This clipped the
ocean's low tide at its component datum.

The sea-state subsystem now preserves the authored ocean transform and height
offset, rebases the ocean actor downward only as much as needed to keep its
component offset non-negative across the maximum 14 cm spring-tide swing, and
applies the signed tide to that rebased offset. Effective surface height remains
`authored actor Z + authored component offset + Island tide`. Teardown restores
the original transform and component offset.

## Validation

- UE 5.8.3 isolated editor target built successfully with `-NoHotReloadFromIDE`.
- `CaptiveSky2.Agent.DayNight` and `CaptiveSky2.Agent.DayNightPersistence`
  passed. The tests now specify equinox assumptions explicitly, tolerate float
  error at sunset, and sample crescent geometry at the disc equator rather than
  its pointed poles.
- `CaptiveSky2.Agent.IslandSeaState` passed, including spring high, low water,
  non-finite sea recovery, and authored offset/transform restoration.
- `CaptiveSky2.Agent.TideglassTide` passed.
- Runs used `-NullRHI`, disabled agent thinking, and set model requests to zero.
  They validate code and state, not rendered water appearance, editor visuals,
  GPU memory, or the 30-fps gameplay target. Those remain to be checked in a
  rendered session.

Logs: [`day/night`](../../Saved/Logs/Codex_Tide_DayNight_Final_20261101.log),
[`sea state`](../../Saved/Logs/Codex_SeaStateRegression_Rebased_20261101.log),
and [`Tideglass`](../../Saved/Logs/Codex_TideglassRegression_20261101.log).
