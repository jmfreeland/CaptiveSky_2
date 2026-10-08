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

An isolated UE 5.8.3 standalone Game render reached world-ready in 30.9 seconds
and captured the pool at Day 6, 12:00, with a 60-second runtime cap and zero
model requests. The image confirms the water material and shoreline are
rendering together at the expected location:
[`001_Tideglass_Lunar_Comparison.png`](../../Saved/CompileScratch/Codex_TideglassLunarValidation_20261008/Project/Saved/Playtests/TideglassHigh/Screenshots/001_Tideglass_Lunar_Comparison.png).

The matched Day 24, 12:00 low-tide run did not reach world-ready within the
120-second startup timeout. Its log stops during Unreal's Turnkey platform
startup; the platform-validation helper itself completed successfully, and the
spectator harness terminated only that scratch Game process. No low-tide
image was produced, so a matched rendered comparison is still needed before
claiming the 14 cm range reads naturally against the shore shelf. The user's
interactive editor remains open with an unsaved level and was not modified.
