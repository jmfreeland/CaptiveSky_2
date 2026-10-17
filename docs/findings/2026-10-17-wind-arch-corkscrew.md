# WindArch corkscrew airflow cue — 2026-10-17

Raven has repeatedly asked for varied wind and changing weather, and his memory
records wanting a few genuinely wild corners where discoveries can happen. The
WindArch already answers an inspection with three temporary glowing motes, but
the previous trajectory moved each mote along a straight line using wrapped
phase offsets. Those offsets could jump from the downwind edge to the upwind
edge before the short gust expired, and conveyed direction more than depth or
variation.

The response now sends the three motes along distinct, phase-shifted 3D
corkscrew paths around the sampled local wind direction. They progress
monotonically from the upwind boundary to the downwind boundary, retain only a
small flutter around the broad arc, and then use the existing fade and
eighteen-second lifetime shared with the local gust. The change is local to
the temporary landmark cue: it does not alter IslandWeather, the global wind
field, resident request limits, or the saved Island.

Automation coverage now checks that the three paths remain spatially distinct,
that a short time step moves forward without a wrap/pop, that the cue still
expires, and that WindArch's interaction text reports the shared eighteen-
second lifetime. The UE 5.8.3 editor target compiled successfully in the
separate `Codex_VisitorResonance_20261017` scratch binaries, including both
changed translation units. The focused `CaptiveSky2.Agent.NightEcology` test
passed earlier under NullRHI, with resident thinking disabled, zero model
requests, and an isolated data root; its run log is
`Saved/CompileScratch/Codex_VisitorResonance_20261017/Saved/Logs/Codex_WindArchCorkscrew_NightEcology_20261017.log`.
The follow-up test launch after adding the explicit duration assertion did not
produce a new log, so that assertion is compile-checked but not claimed as
runtime-verified.

The user's unresponsive UnrealEditor PID 22688 remains running and was not
terminated. A real-RHI visual check remains required to judge whether the helix
reads as a graceful current or as an overly mechanical corkscrew. No visual or
gameplay claim is made from the NullRHI test.
