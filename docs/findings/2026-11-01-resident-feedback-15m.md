# Fifteen-minute resident feedback run — 2026-11-01

## Run conditions

The isolated `Saved/CS58` scratch project was run with
`Start-Spectator.ps1 -NullRHI -NullRHICharacterFeedback`, agent thinking enabled,
Python disabled, situation logging enabled, a 900-second real-time cap, a
40-request ceiling, and a fresh `DataRoot` at
`Saved/CS58/Saved/Playtests/Codex_ResidentFeedback_20261101/`. The scratch
project's source points to this worktree and its content junction points to the
main D: project's assets. The main project's editor remained separate; no
rendered PIE/Game session was started.

The Island reached world-ready in 31.5 seconds. The supervisor ended the run
normally at the 900-second play cap (process exit code 0). The log records 36
decision requests and four optional check-ins, all delivered after 745–747 real
seconds; the request ceiling was not reached. The chronicle contains 36
decisions: Aster 10, Raven 10, Rat 8, and the Innkeeper 8. No
`request_upgrade` or `request_object` action was recorded.

## Resident observations

Aster and Raven began a short, reciprocal exchange after noticing a minnow
break the surface. At the later voluntary check-in, Aster said: “The rain is
making its own little rings on the pool now. I like that the minnows keep
circling back together after a ripple passes.” Raven, Rat, and the Innkeeper
chose not to speak; the invitation left their other choices available. Raven
later echoed Aster's observation, and both added another line about the
rain-rippled pool. Their conversation is present in the isolated agent memories.

Other observed choices were gentle and varied: Rat listened to the
ListeningStones and later chose the rain-basin interaction; Raven watched the
dragonfly; Aster quietly observed the fox, minnows, and pool; the Innkeeper
eventually wandered to a different area. The chronicle also records two worn
trail regions and a rainbow after the shower. These are in-world NullRHI events,
not evidence of rendered readability.

The basin's isolated end state has water level 0.989 and no saved leaves, so the
Rat's selected interaction did not leave a persistent leaf. The log does not
establish whether the basin was still below its float threshold when he acted;
check the precise basin result on a future wet-basin pass before changing the
interaction.

## Decision and next step

Keep the resident feedback invitation optional. This run produced a specific,
useful observation about the rain and minnows, while the three silent choices
were respected. Do not tune prompts to force requests: none accumulated during
this session. The next useful playtest should begin with enough rainwater in the
basin to verify the leaf interaction and, once GPU headroom permits, pair that
with the player-controlled shoreline/woodland visual pass.

This NullRHI session provides no frame-time, VRAM, rendered vegetation, or
player-movement evidence. Its chronicle, memories, trail changes, and basin
state remain isolated under the scratch playtest `DataRoot`; they were not
merged into the main Island save.

Log: [`Codex_ResidentFeedback_20261101.log`](../../Saved/CS58/Saved/Logs/Codex_ResidentFeedback_20261101.log).

## Isolated follow-up attempt — 2026-11-01, 05:55 UTC

A fresh `Start-Spectator.ps1` attempt used the scratch `Saved/CS58` project,
`-NullRHICharacterFeedback`, Python disabled, situation logging enabled, a
900-second cap, a 40-request ceiling, and a new isolated `DataRoot`. The engine
exited after about 25 seconds, before the Island's world-ready marker; the
supervisor treated exit code 0 as failure. No resident session, model request,
or playtest state file was produced.

The fresh crash context is the same known startup failure, not evidence of a
new memory issue: null-address `EXCEPTION_ACCESS_VIOLATION`,
`SecondsSinceStart=0`, and `PCallStackHash
4DCE2FEABCC3C947BBCB4179F07C8373B60F798D`, matching the 2026-10-31
object-feedback crash. The dump remains unsymbolized with UnrealEditor-Core
frames. Artifacts:
[`crash log`](../../Saved/CS58/Saved/Crashes/_0000/Codex_ResidentFeedback_20261102.log)
and [`CrashContext.runtime-xml`](../../Saved/CS58/Saved/Crashes/_0000/CrashContext.runtime-xml).

Do not retry this NullRHI launch unchanged. Resume character feedback only
after symbolized dump analysis or a materially different startup path; the
15-minute run documented above remains the latest successful feedback sample.
