# Listening Stones wind-shaped resonance — 2026-10-16

The Listening Stones' finite chime now uses the local horizontal wind vector,
not only its speed. Wind speed retains the existing subtle rise of up to 1.5
semitones; alignment with the standing-stone forward axis adds at most +/-0.35
semitones. Crosswind and calm conditions leave this directional component
neutral. The scalar overload remains available to callers without a direction
sample, preserving its prior speed-only behavior.

Both resident-triggered interaction and the natural-gust presentation pass the
sampled vector into the sound generator. The existing 2.8-second duration,
spatial attenuation, cooldown, short-lived resident context, light response,
and nonpersistent/no-model-call behavior are unchanged. The feature is purely
auditory and does not alter the map, collision, navigation, or landscape.

The UE 5.8.3 scratch editor target compiled successfully. The
`CaptiveSky2.Agent.NightEcology` and
`CaptiveSky2.Agent.ListeningStonePresentation` NullRHI tests both passed,
including forward/reverse/crosswind pitch bounds and verifying that a natural
gust's sampled direction reaches the generated chime. Logs:
`Saved/CompileScratch/Claude_Props/Saved/Logs/Codex_StoneWindDirection_20261016.log`
and
`Saved/CompileScratch/Claude_Props/Saved/Logs/Codex_StoneWindDirection_Presentation_20261016.log`.

This is behavior-level validation only; pitch balance and audibility have not
been reviewed in a real-RHI game session. The open editor was left untouched.
Next, listen to a short capped zero-request Game run under contrasting wind
directions, then continue the separate mid-distance species work only after an
editor-safe Megaplant export candidate is available.
