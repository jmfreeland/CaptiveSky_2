# Raven storm-wrack foraging (2026-10-09)

Fresh storm-fallen driftwood is now an optional second source for the Raven's existing `GatherTwigs` action. A grounded Raven can gather one small bundle of loose branchlets from visible driftwood within 2.5 m when no established arrangement-site twig pile is available. The larger piece disappears from the shore ledger; carrying the bundle does not require weaving a nest. Existing arrangement-site forage remains the preferred source and keeps its once-per-site-per-day renewal rule.

This is an offered affordance, not an assigned chore. The Raven's already-scheduled situation prompt can mention an untouched distant driftwood fall alongside its shorefall target. It neither selects a target nor causes a model request. Once close, the same build-options description offers the choice. Residents' local wrack observations remain local.

Only fresh, unturned driftwood can be gathered. A turned item—and whatever a resident found beneath it—stays on the shared shore for others to discover. Kelp, shells, and glass floats cannot be mistaken for nest material. The removal is written to the existing wrack JSON ledger before the Raven receives the bundle or its transient actor is removed. If storage is disabled, missing, or fails, the ledger is restored and the Raven carries nothing. No schema bump is needed: foraging is represented by the existing item's absence, and monotonically increasing IDs prevent a later stormfall from reusing it.

## Verification

- UE 5.8.3 isolated scratch editor target built successfully (`Codex_WrackRavenForage_BuildVerified_20261009.log`).
- `CaptiveSky2.Agent.IslandWrack` passed, including pure ledger tests for untouched-driftwood eligibility, preserving turned discoveries and unrelated wrack, and removal surviving serialization.
- `CaptiveSky2.Agent.IslandWrackBottleInteraction` passed, guarding the existing shore interaction.
- `CaptiveSky2.Agent.IslandWrackRavenForage` passed: the Raven cannot gather in flight; once grounded, it carries a bundle, the driftwood ledger entry and actor disappear, unrelated float wrack remains, and reloading the isolated ledger keeps the driftwood absent. A deliberate invalid-destination probe confirmed that failed persistence grants no bundle and restores the driftwood and its actor.
- Run used `NullRHI`, resident thinking disabled, maximum model requests set to zero, and an isolated data root. Logs: [`build`](../../Saved/CompileScratch/Claude_Props/Saved/Logs/Codex_WrackRavenForage_BuildVerified_20261009.log), [`automation`](../../Saved/CompileScratch/Claude_Props/Saved/Logs/Codex_WrackRavenForage_AutomationPassed_20261009.log).

## Limits and next check

The tests prove local availability, safe persistence and action state; they do not prove the Raven autonomously chooses to fly to shorefall or that the driftwood-removal/carry transition reads clearly in the live map. Next, stage a bounded real-RHI Game capture with the actual Raven and fresh shore wrack, then observe whether the optional cue supports self-directed action without repeated foraging or excess model calls. Keep the existing real-time and request caps in force.
