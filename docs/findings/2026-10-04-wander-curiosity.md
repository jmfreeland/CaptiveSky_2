# Wander curiosity respects each resident's recent inspections

## Change

Ground and flight wandering now consider a visible static landmark for their curiosity preference only when that resident has not successfully inspected it within its existing inspection cooldown. Hidden actors, living actors, and landmarks without a specific target tag are excluded. Each controller uses its own inspection history; no shared world knowledge or persistent memory is introduced.

The common eligibility check is used by both `AAutonomousAgentAIController` and `ARavenAgentAIController`. It only filters the optional landmark-progress signal: it does not remove a landmark from direct interaction, guarantee a discovery, or change pathfinding, visibility checks, wander destinations, the raven's 40% curiosity roll, candidate limits, or request/play budgets. Existing inspection cooldowns remain authoritative (300 seconds for ordinary landmarks, with the hearth's shorter existing cooldown).

## Evidence and validation

Deterministic automation coverage checks visibility, landmark identity, living-actor exclusion, per-resident history, cooldown expiry, and malformed timestamps. The blocked-move integration fixture performs a real Tideglass inspection and checks that the controller-level gate then rejects that landmark. Raven flight target selection checks curiosity and random fallback.

The isolated UE 5.8.3 editor target compiled and linked successfully. On 2026-10-04, bounded no-model editor runs passed `WanderCuriosity`, `BlockedGroundMoveApproach`, `ResidentWanderPaths`, `RavenPerch`, and `SessionSafety` (each reported `Result={Success}`). The movement fixture uses the raven controller subclass, so the inherited controller gate is exercised with actual inspection history; the raven perch suite covers the existing aerial curiosity/fallback selection.

A separate isolated 17:00 spectator run loaded the scratch module with a 180-second ceiling and four-request maximum. It ended after 79 seconds when all four requests had been used, produced eight viewpoint screenshots, and recorded a walker reaching another resident before a later route was blocked; the raven completed a perch landing. This confirms the world and both movement controllers ran in a capped live session, but it does not isolate whether recent inspection changes future wander choices. The log still showed one HTTP request in progress during shutdown, so completion of that final response is unknown. Its data root was under the scratch project; the selected [Wind Arch sunset highlight](../../Saved/Highlights/2026-10-04/1926/Wind-Arch-Raven-at-Sunset.png) was copied into the main highlight folder.

## Next check

For a focused follow-up, arrange a bounded session where one resident successfully inspects a landmark, then has a later wander decision while the existing cooldown is active; compare another resident's choices around the same landmark. Preserve the existing session request limits and the 30-minute real-time play ceiling, and leave enough request budget for the final response to settle before shutdown.
