# Resident proposal feedback loop

Resident proposals already entered a human-review inbox, but the live decision path never read later status changes back to the requester. This made requests a one-way interaction.

`IslandAssetRequestQueue::ReadReviewedOutcomes` now reads completed review states for one resident, validates the inbox before returning anything, and fails closed on malformed records. During the resident's next normal thought—after the existing session budget reserves that already-authorized model request—the brain records a durable memory tagged by request ID and status, then includes up to the three newest outcomes in the situation. A status transition is delivered once; a later `accepted` → `generated` or `accepted` → `declined` transition creates a distinct memory/update. No extra model request is created, and this path does not invoke asset tools or modify the level.

The wording is deliberately precise: `accepted` is approval for consideration, `generated` does not claim an asset was imported or placed, and `declined` reports that no work is planned from that proposal. The proposal description is treated as untrusted data. Updates are filtered by exact requester ID; a resident cannot see another resident's review.

Automation coverage was added for requester filtering, pending-state omission, malformed-inbox fail-closed behavior, memory-tag persistence, one-time delivery, and subsequent status transitions. The UE 5.8.3 `CaptiveSky_2Editor` target built and linked successfully on 2026-10-11 (16 actions). At the time, the connected editor loaded the older `captivesky_2_claude` module, so these new automations were not run in that session; its test registry was not used as runtime evidence. Main-tree verification follows below.

## Main-tree automation verification

On 2026-10-11, both requested automation tests ran successfully from the
main-tree `UnrealEditor-Cmd.exe` under NullRHI. `CaptiveSky2.Agent.MemoryComponent`
passed the review-message wording, durable memory tags, one-time delivery, and
later status-transition checks. `CaptiveSky2.Agent.AssetRequestQueue` passed
requester scoping, pending-state omission, and malformed-inbox rejection. Each
was a separate bounded launch with resident thinking disabled, an isolated
`-CaptiveSkyDataRoot`, a 60-second real-time limit, and a zero-model-request
limit; each log reports `Result={Success}` and one test performed:
[`MemoryComponent`](../../Saved/Playtests/Codex_ProposalFeedbackVerify_20261011/Logs/MemoryComponent.log),
[`AssetRequestQueue`](../../Saved/Playtests/Codex_ProposalFeedbackVerify_20261011/Logs/AssetRequestQueue.log).

This verifies the proposal-feedback mechanics in the main-tree module, not a
live resident's next provider-backed thought. The separate connected editor
may still have the older Claude worktree module loaded; no conclusion about
that process is needed for these commandlet results.
