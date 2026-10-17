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

## Isolated runtime validation follow-up (2026-10-17)

The project was copied into an ignored scratch directory with its own `Binaries`,
`Intermediate`, `Saved`, and DDC while linking `Source` and `Content` back to the
working tree. The UE 5.8.3 editor target built and linked there successfully
(173 actions, 240.31 seconds), leaving the open editor's DLL untouched.

Launching that scratch project with `UnrealEditor-Cmd -NullRHI` for the focused
automation test crashed before the test ran: `EXCEPTION_ACCESS_VIOLATION` reading
null, with crash hash `F405610C114D120D4B6F5ABA901EC469AC9FAA26`. This matches the
previously observed project-bootstrap NullRHI crash. No second launch using the
same startup path is warranted. The focused automation and in-game audio/light
review therefore remain outstanding; successful compilation is the only new
validation evidence. The user's main editor process was left untouched.
