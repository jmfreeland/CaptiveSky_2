# Wander curiosity respects each resident's recent inspections

## Change

Ground and flight wandering now consider a visible static landmark for their curiosity preference only when that resident has not successfully inspected it within its existing inspection cooldown. Hidden actors, living actors, and landmarks without a specific target tag are excluded. Each controller uses its own inspection history; no shared world knowledge or persistent memory is introduced.

The common eligibility check is used by both `AAutonomousAgentAIController` and `ARavenAgentAIController`. It only filters the optional landmark-progress signal: it does not remove a landmark from direct interaction, guarantee a discovery, or change pathfinding, visibility checks, wander destinations, the raven's 40% curiosity roll, candidate limits, or request/play budgets. Existing inspection cooldowns remain authoritative (300 seconds for ordinary landmarks, with the hearth's shorter existing cooldown).

## Evidence and validation

Deterministic automation coverage checks visibility, landmark identity, living-actor exclusion, per-resident history, cooldown expiry, and malformed timestamps. The blocked-move integration fixture performs a real Tideglass inspection and checks that the controller-level gate then rejects that landmark. Raven flight target selection retains its existing tests for curiosity and random fallback.

The isolated UE 5.8.3 editor target compiled and linked successfully on 2026-10-04. The follow-up automation-test launch was rejected by Codex's usage-limit approval service before execution, so no result is claimed for this revision. The open main editor has not loaded the updated module; it needs a normal editor restart/build before live behavior can be observed. No LLM-driven wandering run has verified changes in resident behavior yet.

## Next check

After loading the new module, run a short, bounded no-model movement test and then a capped live observation. Confirm that a resident's own recent inspection no longer pulls its wander toward that landmark, while another resident can still be curious about it. Preserve the existing session request limits and the 30-minute real-time play ceiling.
