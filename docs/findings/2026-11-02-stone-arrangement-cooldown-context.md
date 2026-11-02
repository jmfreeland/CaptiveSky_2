# Avoid repeating unavailable stone work

The 2026-11-02 isolated resident chronicle showed the Innkeeper choosing a
second stone-build action at the same arranging ground. The world-state rule
allows one lasting arrangement or response per resident per Island day, so
repeat attempts leave the landscape unchanged.

`UAgentBrainComponent::BuildSituationSummary` now checks that same daily
eligibility before presenting an empty arranging ground as a `build` target.
Once the resident has used the day's stone-work opportunity, nearby empty
grounds remain valid movement/landing destinations, but the situation says
the stones can be left undisturbed until a later Island day and omits the
unavailable build target. This does not change cooldowns, persistent state,
stone placement, or other build actions.

The focused `CaptiveSky2.Agent.IslandArrangement` regression now covers the
post-arrangement prompt: movement/landing remains available, the cooldown is
explained, and the empty ground is no longer offered as a build action. The
UE 5.8.3 `CaptiveSky_2Editor` target compiled both changed translation units
successfully. Automation was not run because the known Unreal startup crash
remains unresolved; this is compile evidence, not runtime behavior evidence.
