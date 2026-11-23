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

## Physical delivery path and limits

A follow-up gameplay path offers Aster a visible, tagged `BlueJar` prop to pick
up, attaches it to the resident's hand socket when available (with a body-root
fallback), and lets her set it down or carry it to `InnCounter`. Delivery
requires the Innkeeper to be nearby and have line of sight to the placed jar.
The ledger records fulfillment only after placement and a successful save; if
the save fails, the jar returns to Aster's hand when possible. A fulfilled ledger
entry restores the authored jar to the counter after a level reload.

The Island map still has no saved `BlueJar` actor. Run
`Scripts/Place-IslandBlueJar.py` in the editor after the map is no longer open
elsewhere; it places the imported mesh on `Inn_Table_Front`, tags it `BlueJar`,
and saves the map. The current editor holds the shared Island map open, so this
placement has not been applied yet. Content and map assets are gitignored; the
tracked placement script makes the local setup repeatable. The gameplay code
compiled, but has not been exercised in-game. It assumes one authored jar and
tagged `IslandInn`/`InnCounter` actors. The natural-language promise check
remains a narrow safeguard for this prototype, not proof that every promise is
sincere.

`CaptiveSky_2Editor` built successfully with UE 5.8.3 (15 actions) after the
physical-delivery follow-up; no new automation test has been added or run.
