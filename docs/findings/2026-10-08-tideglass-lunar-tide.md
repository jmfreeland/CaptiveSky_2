# Tideglass follows a small lunar tide (2026-10-08)

The temporary play-session water surface now rises and falls with the existing
Island clock. A 24.84-Island-hour lunar day gives two high tides per Island day;
the range is gently larger near new/full moon and smaller around quarter moon.
The maximum displacement is 14 cm from the authored resting level. The update
runs twice per simulated second, touches only the transient procedural water
mesh, and requires the level's existing `IslandDayNight` clock. It does not move
the ocean, shore rocks, or blockout pool footprint; it adds no saved field,
model call, or scheduled resident action. The authored pool returns unchanged
when the play world ends.

`CaptiveSky2.Agent.TideglassTide` checks the bounded offsets, semidiurnal high/
low pair, spring-versus-neap range across sixty Island days, and an integration
fixture that verifies the same transient procedural surface rises and falls
with the clock. The UE 5.8.3 `CaptiveSky_2Editor` scratch target compiled and
linked successfully. The NullRHI automation passed with agent thinking disabled,
zero model requests, a 60-second cap, and an isolated data root. Log:
[`TideglassTide.log`](../../Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/TideglassTide.log).

A rendered Game/PIE check is still needed to ensure the small waterline motion
reads naturally against the existing shore shelf rather than looking like a
floating sheet. The user's interactive editor remains open with an unsaved
level, so the saved map was not launched, saved, or otherwise touched for this
visual check.
