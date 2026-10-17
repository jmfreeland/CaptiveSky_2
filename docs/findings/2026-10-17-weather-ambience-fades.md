# Weather ambience gain fades — 2026-10-17

The procedural wind and rain beds already followed sampled weather and listener
shelter, but their volume multipliers were assigned only at the 0.25-second
weather-sampling interval, and a missing listener or calm condition stopped a
bed immediately. The update now retains the same bounded target gains and
weather sampling cadence, while moving each audio component's current gain
toward its target every tick. A bed fades to silence before stopping; its
procedural PCM is replenished only while it is playing. No new actors, assets,
persistent state, LLM calls, or weather behavior were added.

`InterpolateAmbienceGain` clamps both endpoints and the frame delta, blends at
3.5 per second, and cannot overshoot. The existing 0.055 wind and 0.035 rain
gain ceilings and one-fifth indoor shelter multiplier are unchanged.

## Validation

- UE 5.8.3 editor target built successfully with 173 actions in the established
  `Saved/CompileScratch/Claude_Props` project, whose Source/Content junctions
  point to this checkout but whose Binaries are separate from the interactive
  editor. The build did not replace the main project's editor DLL.
- `CaptiveSky2.Agent.IslandWeather` passed under NullRHI. Python and agent
  thinking were disabled; `-CaptiveSkyMaxModelRequests=0` was set. The log ends
  with `TEST COMPLETE. EXIT CODE: 0` at
  `Saved/Logs/Codex_WeatherAmbienceFades_20261017.log`.
- The focused assertions cover fade-in and fade-out steps, target convergence
  without overshoot, and negative frame-delta handling, alongside the existing
  gain ceilings and indoor attenuation checks.

This verifies the gain transition math and code path, not perceived audio
quality. A short real-time listen across calm, wind, rain, shelter entry, and
listener removal is still needed when the interactive editor/game is available.
