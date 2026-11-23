# Social commitment ledger — 2026-11-23

## What changed

`UIslandWorldStateSubsystem` now stores a bounded ledger for one whitelisted action,
`bring_object_to_place`. Each record has a stable ID, proposer and recipient, object
and place IDs, source conversation ID, creation/deadline Island days, current state,
and recipient-witnessed outcome fields. The world-state schema is version 3; files
without `social_commitments` continue to load.

The Island clock advances commitments deterministically: they become due on their
deadline and expire on a later Island day if still unresolved. State changes save
with the world state and add chronicle entries. A resumed saved clock also advances
the ledger. Repeated calls do not duplicate transitions. At 32 records, a new
promise can replace the oldest fulfilled or expired record; active promises are
never discarded to make room. Deadlines are limited to 30 days.

Fulfillment is recorded only when the proposer reports the named recipient as the
witness and supplies concise evidence. The API contract requires its caller to
confirm the object is physically at the promised place; this layer does not create
inventory, move props, or verify Unreal actors.

## Scope and remaining integration

This is the canonical persistence and deadline layer for the November 22
worldbuilding proposal. It does not yet let residents create promises during
delivered speech or choose a delivery action. The speech-delivery files are listed
as active work in `COORDINATION.md`, so that integration remains a coordinated next
step. Until then, the subsystem API is a foundation and no resident-facing behavior
should be claimed as shipped.

`CaptiveSky_2Editor` built successfully with UE 5.8.3 (13 actions, 68.29 seconds).
The focused `CaptiveSky2.Agent.IslandSocialCommitment` automation test passed under
NullRHI with exit code 0; its log is `Saved/Logs/Codex_SocialCommitment_20261123.log`.
Coverage includes validation, deadline transitions, witnessed fulfillment, expiry,
persistence, and loading a legacy version-2 file. This verifies the storage and
state-transition layer; it does not verify the resident-facing speech-to-delivery
loop or a live Island presentation.
