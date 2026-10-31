# Trail wear follows the active view

Visible worn-ground patches are refreshed around the first player's active view point, not only around the pawn. This keeps the bounded wear layer aligned with what a spectator camera or camera cut actually shows. The pawn position remains the fallback when the view is invalid; if there is no pawn, the controller's view target supplies a fallback origin.

The implementation reuses the existing 1.5-second refresh cadence, 55 m draw radius, and 360-instance cap. It changes no saved trail data and adds no per-frame work. `IslandTrailTests.cpp` covers camera preference and invalid-view fallback.

The UE 5.8 editor target compiled and linked successfully against the isolated `Saved/CS58` project copy. The focused automation test and rendered behavior remain unverified: on restart, Unreal opened a `Wait for ZenServer?` prompt, and that prompt is awaiting the user's choice.

Next: after the editor opens, run the focused trail automation once, then inspect wear while viewing from a detached spectator/camera-cut location. Do not expand the draw cap without measured GPU headroom.
