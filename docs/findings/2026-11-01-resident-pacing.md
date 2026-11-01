# Resident pacing after a bounded 15-minute session

## Change

Background autonomous thinking now has a 240-second minimum interval (raised from 120 seconds). Automatic resident-to-resident replies are being slowed from 25 to 60 seconds between lines; this affects only automatic exchanges, not player-directed conversation. The latter change is in source but still needs a successful rebuild and focused automation run.

## Evidence and validation

The earlier isolated 15-minute baseline logged 36 background decisions plus four optional feedback prompts. After the background interval change, an isolated UE 5.8.3 NullRHI spectator session ran for exactly 900.2 real seconds, ended cleanly, and used 25 of its 32 allowed model requests. It remained isolated under `Saved/Playtests/Codex_ResidentPacing_20261101`; no canonical world state was changed. Residents completed two parallel four-line conversations and a later four-line Raven–Aster exchange. No object-upgrade requests accumulated. Raven attempted a leaf interaction, but the controller rejected it as a recent repeat inspection; the saved basin state has no leaves. Raven's same-response reflection nevertheless claimed the leaf had been set afloat. The memory timing defect and tide-awareness follow-up are recorded in [tide awareness and verified interaction memories](findings/2026-11-01-tide-awareness-and-verified-memories.md). All memories/chronicle entries remained in the isolated data root.

The background interval change and its safety automation had already built and passed in the scratch project. After the reply-pause increase, the isolated UE 5.8.3 editor target compiled successfully with `-NoHotReloadFromIDE`, keeping output in the C: scratch project and sending no patch to the open D: editor. The focused `UnrealEditor-Cmd` automation launch then crashed before engine startup (`SecondsSinceStart=0`, null access violation, hash `F405610C114D120D4B6F5ABA901EC469AC9FAA26`), matching the previously recorded headless bootstrap crash. Neither `SocialPacing` nor `SessionSafety` ran against this newly compiled target. Do not repeat the same commandlet startup until that bootstrap issue is diagnosed or the environment meaningfully changes.

## Zen startup diagnosis (2026-11-01)

The active D: editor log records `Local ZenServer AutoLaunch initialization
completed in 52.602 seconds`. The server process started at 07:54:59.231 UTC
and bound port 8558 one millisecond later, but its persisted-session service did
not report loading 1,000 sessions until 07:55:23.350. Unreal did not report Zen
healthy until 07:55:51.698. Rotated Zen logs from the same day show the same
1,000-session load beginning 22–29 seconds after the listener starts. The
sessions directory contained 1,000 directories and 2,000 files (31.9 MiB),
modified from 2026-09-30 through 2026-11-01. After startup, the editor's DDC
probe reported 0.01-second speed tests and 0.02 ms latency.

A bounded `zenserver.exe --powercycle` comparison in isolated Temp data roots
strengthened the lead: a fresh data root initialized in 0.17 seconds with zero
sessions; a second root containing a copy of the 2,000 session files took 33.5
seconds and logged that it loaded 1,000 sessions. This is not a full Unreal
launch or a proof that session restoration is the only cause, but it shows that
the accumulated session state alone reproduces most of the delay. The real Zen
data and DDC were not changed.

`zen.exe service status` reports that a Windows service is not installed; the
editor launches Zen with its PID as owner. `LimitProcessLifetime=False` is
present in the project config, but its effect on this editor-owned process has
not been verified. Do not delete the persisted session files or clear the DDC.
The UE 5.8 [Multi-User Server UI](https://dev.epicgames.com/documentation/unreal-engine/multi-user-server-user-interface-in-unreal-engine)
can list active and archived sessions and offers explicit archive, restore, and
delete actions. First identify whether these records are user-needed Multi-User
history; only consider deleting individually confirmed stale sessions, since
the history may contain unpersisted transactions. Do not bulk-remove it as a
startup optimization.
Source logs: the active project's `Saved/Logs/CaptiveSky_2_2.log` and
`%LOCALAPPDATA%\UnrealEngine\Common\Zen\Data\logs\zenserver.1.log`.

## Next check

Once Live Coding is inactive, rebuild the scratch editor target and run `CaptiveSky2.Agent.SocialPacing` plus `CaptiveSky2.Agent.SessionSafety`. In a later bounded play session, confirm the new one-minute gap makes autonomous conversations feel less bursty without starving them.
