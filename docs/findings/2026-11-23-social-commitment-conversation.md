# Resident promise conversation hook — 2026-11-23

## What changed

The resident decision schema can optionally carry one whitelisted promise payload:
Aster may tell the Innkeeper that she will bring `BlueJar` to `InnCounter` tomorrow.
The social handoff records it only when the line is accepted through the nearby,
reserved resident conversation path and the delivered text explicitly contains
“bring”, “jar”, and “tomorrow”. The world-state ledger remains the authority for
the record and deadline.

Both participants receive the record in their own internal situation summary,
including its pending, due, witnessed-fulfilled, or expired status. Other residents
and external correspondents do not receive this private context. The summary says
that it records speech and does not prove the jar exists or was delivered.

## Limits and verification

This connects the proposal to resident conversation but does not create a jar,
place it on the counter, or let either resident perform a delivery action yet. The
word checks are a narrow safeguard for this single prototype, not general natural
language understanding or proof that every promise is sincere.

`CaptiveSky_2Editor` built successfully with UE 5.8.3 (14 actions). The focused
ledger automation was previously run and passed as recorded in
`2026-11-23-social-commitment-ledger.md`; no new automation test was run for this
conversation hook.
