# Wind Arch motes enter nearby resident perception (2026-10-08)

When the transient Wind Arch light motes are within 14 m of a resident and the
Arch, and an unobstructed visibility trace reaches the effect, the resident's
next already-scheduled situation summary describes the pale lights drifting
with the wind. It explicitly leaves watching optional. The cue is omitted when
the motes are too far away or have been destroyed. This does not create an
action, discovery, memory, saved-world change, or model request.

The isolated UE 5.8.3 editor target built successfully, and
`CaptiveSky2.Agent.WindArchPresentation` passed headlessly with resident
thinking disabled and model requests capped at zero. The automation creates a
transient natural-wind mote pass and verifies nearby perception, distance
filtering, and cue expiry. Log: [`Codex_WindArchMotePerception_Final_20261008.log`](../../Saved/Logs/Codex_WindArchMotePerception_Final_20261008.log).

This confirms the bounded context cue, not whether Aster or the raven chooses
to pause for it in ordinary play or whether the lights read clearly at normal
gameplay distance.
