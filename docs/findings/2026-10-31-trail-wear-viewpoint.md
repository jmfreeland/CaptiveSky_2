# Trail wear follows the active view

Visible worn-ground patches are refreshed around the first player's active view point, not only around the pawn. This keeps the bounded wear layer aligned with what a spectator camera or camera cut actually shows. The pawn position remains the fallback when the view is invalid; if there is no pawn, the controller's view target supplies a fallback origin.

The implementation reuses the existing 1.5-second refresh cadence, 55 m draw radius, and 360-instance cap. It changes no saved trail data and adds no per-frame work. `IslandTrailTests.cpp` covers camera preference and invalid-view fallback.

The UE 5.8 editor target compiled and linked successfully against the isolated `Saved/CS58` project copy. `CaptiveSky2.Agent.IslandTrail` and `CaptiveSky2.Agent.IslandTrailVisitor` then passed in a NullRHI commandlet run with agent thinking disabled, a zero model-request cap, and an isolated data root (`Saved/CS58/Saved/Logs/Codex_TrailView_20261031_Final.log`, exit code 0). The focused run exposed two stale date-fixture assumptions; the test now uses a copy of the ledger for recovering/unknown-age cases, preserving the independent active-region record assertions. NullRHI verifies the logic and test fixtures, not rendered wear placement.

The GUI restart remains at a `Wait for ZenServer?` prompt, so rendered behavior is still unverified. Next, after the editor opens, inspect wear while viewing from a detached spectator/camera-cut location. Do not expand the draw cap without measured GPU headroom.
