# Resident promise conversation hook — 2026-11-23

## What changed

The resident decision schema can optionally carry one whitelisted promise payload:
Aster may tell the Innkeeper that she will bring `BlueJar` to `InnCounter` tomorrow.
The social handoff records it only after the listener accepts the line into the
nearby, reserved conversation queue (or when a valid final turn closes that
conversation), and the delivered text explicitly contains “bring”, “jar”, and
“tomorrow”. A rejected utterance cannot create a promise. The world-state ledger
remains the authority for the record and deadline.

Both participants receive the record in their own internal situation summary,
including its pending, due, witnessed-fulfilled, or expired status. Other residents
and external correspondents do not receive this private context. The summary says
that it records speech and does not prove the jar exists or was delivered.

Commitment deadlines now advance at the Island day rollover. When saved-clock
resumption is enabled, the clock persists the new day and due/expired transition
together through the existing world-state save; with resumption disabled, the
transition is still advanced for the current session. This avoids waiting for
Aster to attempt an action before the ledger and Chronicle reflect a missed
deadline. The source change is committed as `a5e43425`; UE build and live
rollover behavior remain unverified.

After a BlueJar commitment is fulfilled or expires, the first incoming line of a
later conversation between Aster and the Innkeeper may receive one participant-
specific prompt cue. It distinguishes witnessed delivery from a missed witness
deadline, leaves room to acknowledge the outcome or move on, and does not assign
motive, debt, gratitude, or an obligation to apologize. This adds no persistent
state or model request. The prompt change is not yet built or observed in play.

Creation, due, expiry, and fulfillment Chronicle entries now carry the same
commitment ID and source conversation ID. This lets an observer trace the
outcome back to the recorded exchange without duplicating the spoken line or
implying an unrecorded intent. The readable Chronicle and Atlas timeline show
the lifecycle event and both IDs; fresh Chronicle data and visual review are
still pending.

## Physical delivery path and limits

A follow-up gameplay path offers Aster a visible, tagged `BlueJar` prop to pick
up, attaches it to the resident's hand socket when available (with a body-root
fallback), and lets her set it down or carry it to `InnCounter`. Delivery
requires the Innkeeper to be nearby and have line of sight to the placed jar.
The ledger records fulfillment only after placement and a successful save; if
the save fails, the jar returns to Aster's hand when possible. A fulfilled ledger
entry restores the authored jar to the counter after a level reload.

The shared editor has saved a tagged `BlueJar` actor on `Inn_Table_Front`; its
last placement record is in the editor clone's ignored
`Saved/BlueJarPlacement.json`. Review of the recorded transform exposed a bug:
the initial setup and delivery code aligned the jar's bounds center to the
surface instead of aligning its bounds minimum. The placement script and
runtime set-down, delivery, and reload-restore paths now use the bounds minimum
relative to the actor pivot. Re-run `Scripts/Place-IslandBlueJar.py` in the
editor to repair the existing source placement and save the local Island map.
As of 2026-11-25, the active editor has `/Game/Maps/Island` loaded with no PIE
session, but runs from a detached Claude worktree at `8a65863` (2026-09-29), not
from current `main`. That clone has local changes to `CaptiveSky_2.uproject` and
`Config/DefaultEditor.ini`; preserve them. Its successful 07:20 UTC Nov 23
Live Coding compile cannot validate current `main` source. The ignored
`Saved/BlueJarPlacement.json` in that clone was last written at 10:52 UTC Nov
23, before the surface-bounds correction commit at 10:58 UTC. The correction
has not yet been applied or visually checked. Content and map assets are
gitignored; the tracked script makes the local setup repeatable. A fresh build
from current source and in-game review are still required.
It assumes one authored jar and tagged `IslandInn`/`InnCounter` actors. The
natural-language promise check remains a narrow safeguard for this prototype,
not proof that every promise is sincere.

`CaptiveSky_2Editor` built successfully with UE 5.8.3 (15 actions) after the
physical-delivery follow-up; no new automation test has been added or run.

## Current-main runtime verification — 2026-11-25

The current `main` editor target built successfully with UE 5.8.3 (13 actions)
after a separate PIE crash in `AAutonomousAgentAIController::Think()` was traced
to a null console-variable lookup. The controller now looks up
`CaptiveSky.Agent.DisableThinking` through `IConsoleManager` and checks for a
missing variable before reading it (`c7ee0ff0`). This fixed the observed crash
path; it does not verify the promise or place-name lifecycle.

A standalone spectator session from the current project reached Island
world-ready, ran for 205.5 seconds, issued 12 model requests, and drained all
in-flight requests before exiting successfully. The log showed residents
wandering, approaching landmarks, noticing wildlife, and retaining observations
about the Listening Stones and wildlife. It recorded no conversations,
`place_name_claim` events, or `social_commitment` events. The isolated run found
one Innkeeper observation and two Raven observations; the checked-in resident
memories were not changed. This is evidence of autonomous movement and
observation only, not evidence that a promise was made or kept, nor direct
feedback from a human player or resident.

The visual captures suggest concrete presentation work for a later editor
session: the shoreline reads sparse, Tideglass ground cover is dense, the Inn
camera clips through nearby geometry, and the Listening Stones read as stacked
slabs. These are observations from the captures, not fixes. The UE editor and
map were not modified during this verification.
