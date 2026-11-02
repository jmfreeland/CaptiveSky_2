# Spectator cleanup on viewer disconnect — 2026-11-02

`AIslandSpectatorDirector` intentionally ticks while paused to support its
visitor-facing camera and bounded capture tools. Before this change, a viewer
disconnect could invalidate the director's weak player-controller reference
without calling `EndSpectating()`. The transient director then kept ticking
while paused, and the visitor pawn could remain hidden, non-colliding, and
without input; a later visitor could also be blocked from starting a fresh
spectator because the old director still existed.

`ACaptiveSky_2GameMode::Logout` now asks the world's
`UIslandSpectatorSubsystem` to stop spectator mode for the departing
`APlayerController` before the base logout tears down the controller. The
subsystem checks the director's viewer identity, so an unrelated controller's
logout cannot stop someone else's spectator. The existing `EndSpectating`
cleanup restores the hidden pawn and its collision, movement mode, input, and
view target, then destroys the camera director; its `EndPlay` also stops an
active delayed trace.

`CaptiveSky2.Agent.Spectator` now covers controller-scoped cleanup: another
controller cannot stop the active spectator, while cleanup for its viewer
stops the director and restores the visitor. The UE 5.8.3 Editor target built
and linked successfully (8 actions, four maximum parallel actions). Runtime
automation remains unverified because the current engine startup access
violation occurs before automation discovery. No Unreal launch, map save, or
asset change was made for this change.
