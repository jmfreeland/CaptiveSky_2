# Listening Stones visitor resonance

The transient Listening Stones presentation now responds to a quiet player
approach as well as a natural rising gust. A player-controlled pawn within
3.25 m, no more than 2.4 m above/below the stones, and moving at no more than
2.6 m/s starts the existing short resonance: the fitted stones pulse their
lights and a finite procedural chime plays. The response is latched for that
approach and rearms only after the visitor leaves a 5.75 m ring. Visitor and
natural-gust responses share the existing 45-second cooldown to prevent stacked
chimes.

This is a transient world reaction only. It does not modify the saved landmark
proxies, collision, navigation, resident memory, world state, or model-request
budget. Unpossessed wildlife and other non-player pawns do not trigger it.

The implementation and regression-test translation units both compiled
successfully against UE 5.8.3 with `-NoLink`; the editor DLL was not relinked.
The automation now covers the approach radius, non-player exclusion, one cue
per visit, re-entry suppression, and the shared natural-gust cooldown. The
existing editor process is unresponsive, so the automation test and normal
viewport/game audio-light review remain unrun. Do not treat compile evidence as
proof of in-game audibility or visual quality.
