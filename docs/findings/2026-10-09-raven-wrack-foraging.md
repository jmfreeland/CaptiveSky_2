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
- After the live landing fix, `CaptiveSky2.Agent.IslandNest` passed again in UE 5.8.3 `NullRHI`; its refusal-message assertion now matches the driftwood-specific response. Log: `Saved/Playtests/Codex_RavenForagePresentation_20261009/Logs/RavenIslandNestAutomationPassed.log`.

## Limits and next check

The automated tests prove local availability, safe persistence and action state. A later isolated real-RHI probe now verifies the physical move → land → gather sequence against live storm-wrack; it is deterministic instrumentation, not evidence that the Raven autonomously chooses to forage.

The first real-RHI close capture exposed an art issue: the seed produced one smooth cylinder that read as a pipe, not storm-fallen wood. Driftwood layout now gives each item one or two heavier limbs with shorter snapped branches. The layout remains deterministic, uses the existing instanced cylinder component, and stays inside the existing ten-piece budget. After the change, `CaptiveSky2.Agent.IslandWrack` (including bottle interaction and Raven forage) passed in UE 5.8.3. An isolated, real-D3D12 Game capture rendered the saved shorefall at 1600x900 and ended at the 60-second cap with zero model requests. Screenshot: `Saved/CompileScratch/Codex_RavenWrackVisual_20261009/ScreenshotsBranchwood/001_Storm_Driftwood_Close.png`.

The new silhouette is clearer, but the close image still shows smooth stock cylinders and a conspicuous flat-cut end; this is an incremental improvement, not finished naturalistic driftwood. The bounded Raven capture below now covers the approach and gathering behavior. A textured bark mesh or a suitable free asset is still the next art pass; keep the existing real-time and request caps in force.

## Bounded real-RHI move, landing, and forage

`Island.MoveProbe` can now append the Raven's ordinary `Land` and `GatherTwigs` actions after a physical move. If its movement target is itself a listed `ArrangingGround_*` marker, that marker doubles as the landing target; otherwise, the optional landing-site tag remains explicit. The probe checks the Raven's actual carried-twigs state and queues a post-action screenshot only when the action succeeds.

The first D3D12 run exposed two concrete issues: the isolated test marker was 125 cm below the local landscape, and a swept capsule's expected contact with the traced ground was being reported as a blocked flight. The test marker was corrected from the live landscape hit, and Raven landing now accepts only an upward-facing contact within 25 cm of both the traced landing center and surface height. Other obstructions still fail the landing. A separate warning names any geometry hiding a requested landing marker.

UE 5.8.3 editor target built successfully (`Codex_RavenGatherSuccessFinalBuild_20261009.log`). A 1600×900 D3D12 Game run moved 278 cm to the listed open-ground site, landed, gathered the actual untouched storm-fallen driftwood, and removed its ledger entry. The existing site's daily forage had already been consumed in the isolated fixture, ensuring this action came from driftwood rather than the nearby arrangement-site bundle. Safety was active at 90 real seconds / zero model requests; the run ended after 33.1 seconds with zero requests. Log: `Saved/Playtests/Codex_RavenForagePresentation_20261009/Logs/RavenForagePresentationDaylight.log`. Post-action frame: `Saved/Playtests/Codex_RavenForagePresentation_20261009/ScreenshotsDaylight/Raven_Wrack_GatherTwigs.png`.

The daylight frame makes the grounded Raven readable, but its beak bundle is not obvious at this distance and the stock driftwood is gone by the post-action frame. The screenshot proves the final physical state, not the gathering animation or visible branchlets; a paired before/after capture and a clearer carrying visual remain worthwhile follow-ups.

## Follow-up appearance trials

Three no-cost local trials were inspected in the same isolated real-D3D12 view. The spruce-pack trunk material produced bright striping and dark wedges on the engine cylinder; StarterContent `M_Wood_Oak` showed no useful grain at this scale and left the cut end stark; the existing Tripo `FirewoodStack` mesh rendered as a tidy, pale rectangular stack rather than storm-wrack. None was retained in source. `IslandWrack.cpp` and `.h` are back to the committed branchwood implementation (`859a79a`); scratch captures remain under `Saved/CompileScratch/Codex_RavenWrackVisual_20261009/`.

A free Tripo doctor check confirmed the configured API key and 810 credits available (0 frozen). Its no-credit dry run produced this candidate plan: one text-to-model PBR standard-texture prop using `v3.1-20260211`, capped at 12,000 faces, followed by FBX conversion; estimated total 25 credits (20 generation + 5 conversion). The planned prompt calls for one twisted, salt-bleached coastal limb with three to five snapped branch stubs, not a firewood stack. No generation was submitted and no credits were spent; wait for explicit user approval before submitting. If approved, inspect the returned preview before importing and wiring it into the shore wrack.
