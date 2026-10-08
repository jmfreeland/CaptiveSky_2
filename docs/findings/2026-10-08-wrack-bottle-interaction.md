# Message-in-a-bottle interaction

The bottle inspection now reads from the active world's `UIslandChronicleSubsystem::GetChroniclePath()` rather than opening the project's canonical chronicle path directly. This keeps code-created transient worlds isolated: if a world has no configured chronicle, it cannot accidentally reveal thoughts from the live island's shared Chronicle.

The focused `CaptiveSky2.Agent.IslandWrackBottleInteraction` automation test creates a transient world with unique scratch Chronicle and wrack files, records an earlier Aster decision, and inspects a tagged glass float through `IslandInteractionUtility::Perform` as a Raven observer. It verifies the returned discovery is attributed and time-labelled, the shore ledger records who turned the float, and the opened state and note survive reloading. The test uses only its temporary files and deletes them on completion.

Validation on 2026-10-08: Unreal Engine 5.8.3 `CaptiveSky_2Editor` scratch build succeeded; the focused headless automation test passed (exit code 0). The user's open editor and shared `WorldState` were not used or modified.
