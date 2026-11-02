# Resident choices with nearby object targets — 2026-11-02

## Run conditions

Ran one hidden UE 5.8.3 `NullRHICharacterFeedback` Game session from the
isolated `Saved/CS58/CaptiveSky_2.uproject`. Agent thinking was enabled; Python
was disabled; resident situations were logged; saves went only to the fresh
`Saved/CS58/Saved/Playtests/Codex_ResidentFeedback_ObjectTargets_20261102/`
root. The run had a 300-second startup timeout, a 900-second play ceiling, a
32-request cap, a 4 GiB system-memory preflight, and the existing two-sample
memory monitor. Preflight passed with 9,777 MiB available. World-ready took
37.5 seconds. Zen was already running and answered OK; initialization took
0.010 seconds, so this run does not test a cold Zen launch.

The game was stopped by its RAM guard before the 15-minute play cap. Available
physical memory fell below 4,096 MiB on two checks (4,063 then 3,940 MiB), and
the launcher terminated only the Game process tree. The session therefore
ended with exit code 1 by design, at roughly ten minutes; it is not a completed
15-minute feedback run. Do not use this as rendered, FPS, or GPU evidence.
After the launcher stopped the Game process, the same Windows memory API
reported 9,669 MiB available again, and the process list contained no Unreal
process. This closely associates the temporary pressure with the active
session, but the run did not capture per-process RSS or identify which Game
subsystem accounted for it.

## What residents chose

The isolated chronicle shows several distinct, self-directed choices:

- Rat first approached the Innkeeper, later greeted them when nearby, and
  remarked on the rain.
- The Innkeeper went to an Arranging Ground and chose to make a stone form,
  then later chose another build action at that same target. The chronicle
  records the choices, but this run did not verify whether the second action
  created a distinct persistent arrangement.
- Raven greeted Aster, exchanged a few short observations about the rising
  tide and drifting petals, then chose a quiet pause. Later, as rain began,
  Raven chose to approach the Rain Basin.
- Aster returned the greeting, then gave the conversation space and wandered
  before idling in the rain.

Raven's logged situation included the new resident-relative directions for
three nearby ordinary objects (`Aster` ahead, `Roost_East_MainBranch` to the
left, and `TideglassPool` to the right), each paired with optional
`request_upgrade` guidance. No `request_upgrade` or `request_object` appears in
the saved chronicle. This suggests the added targeting information was
available without dominating the residents' decisions. It does not establish
that they understood every offered upgrade target, or that the rendered props
are visible and recognizable to a human visitor.

## Evidence and next step

The chronicle and agent memories remain isolated under the playtest root. The
full Game log is
`Saved/CS58/Saved/Logs/Codex_ResidentFeedback_ObjectTargets_20261102.log`.
The launcher now records the launched Game PID's working set and private bytes
alongside each five-second system-memory sample when those process metrics are
available. This does not alter the global RAM threshold or process-scoped stop
behavior. The PowerShell system-memory tests pass, including a live-process
snapshot check.

### Follow-up guarded 15-minute memory-profile run

A second hidden session used the same UE 5.8.3 scratch project and safeguards,
with a separate data root at
`Saved/CS58/Saved/Playtests/Codex_ResidentFeedback_MemoryProfile_20261102/`.
Preflight passed with 11,927 MiB available; world-ready took 38.8 seconds. The
run completed its 900.1-second play cap normally with 16 of 32 model requests
and no in-flight calls. Zen was already running and initialized in 0.018
seconds, so this still does not test a cold Zen start.

The Game's initial process sample was only 331 MiB private bytes, then grew
during world load to about 5,396 MiB. Thereafter it was nearly flat: about
5,392–5,403 MiB private bytes and 5,237–5,251 MiB working set through the end
of the run. Available system memory stayed above approximately 6,400 MiB after
world-ready, with no low samples. This materially supports a large but stable
Game startup footprint, rather than steady memory growth over a 15-minute
session. In the earlier interrupted run, preflight began about 2,150 MiB lower;
after allowing for the same roughly 5.4 GiB Game footprint, its eventual
crossing of the 4 GiB reserve is unsurprising. Keep the reserve and monitor;
future runs should retain this per-process trace. No rendered, FPS, or GPU
claim is made.

All four residents answered the optional check-in. The Innkeeper enjoyed rain
stippling the basin and wanted to explore how the Listening Stones answer;
Aster would preserve the weather-made water movement; Rat wanted to see
Tideglass under moonlight; Raven would preserve the quiet changing details.
Earlier in the session, the Innkeeper chose to float a leaf in Rain Basin,
which the chronicle confirms persisted in this isolated world state. No
`request_upgrade` or `request_object` proposal was made. These reflections are
art-direction signals, not proof of what visitors can see in a rendered build.
Do not merge isolated agent memory into canonical saves.

The follow-up log is
`Saved/CS58/Saved/Logs/Codex_ResidentFeedback_MemoryProfile_20261102.log`; its
chronicle and resident memories are beside it under the separate playtest data
root.
