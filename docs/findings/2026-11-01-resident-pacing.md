# Resident pacing after a bounded 15-minute session

## Change

Background autonomous thinking now has a 240-second minimum interval (raised from 120 seconds). Automatic resident-to-resident replies are being slowed from 25 to 60 seconds between lines; this affects only automatic exchanges, not player-directed conversation. The latter change is in source but still needs a successful rebuild and focused automation run.

## Evidence and validation

The earlier isolated 15-minute baseline logged 36 background decisions plus four optional feedback prompts. After the background interval change, an isolated UE 5.8.3 NullRHI spectator session ran for exactly 900.2 real seconds, ended cleanly, and used 25 of its 32 allowed model requests. It remained isolated under `Saved/Playtests/Codex_ResidentPacing_20261101`; no canonical world state was changed. Residents completed two parallel four-line conversations and a later four-line Raven–Aster exchange. No object-upgrade requests accumulated. Raven attempted a leaf interaction, but the controller rejected it as a recent repeat inspection; the saved basin state has no leaves. Raven's same-response reflection nevertheless claimed the leaf had been set afloat. The memory timing defect and tide-awareness follow-up are recorded in [tide awareness and verified interaction memories](findings/2026-11-01-tide-awareness-and-verified-memories.md). All memories/chronicle entries remained in the isolated data root.

The background interval change and its safety automation had already built and passed in the scratch project. After the reply-pause increase, the isolated UE 5.8.3 editor target compiled successfully with `-NoHotReloadFromIDE`, keeping output in the C: scratch project and sending no patch to the open D: editor. The focused `UnrealEditor-Cmd` automation launch then crashed before engine startup (`SecondsSinceStart=0`, null access violation, hash `F405610C114D120D4B6F5ABA901EC469AC9FAA26`), matching the previously recorded headless bootstrap crash. Neither `SocialPacing` nor `SessionSafety` ran against this newly compiled target. Do not repeat the same commandlet startup until that bootstrap issue is diagnosed or the environment meaningfully changes.

## Zen startup observation

The local Zen install and data directories exist under `%LOCALAPPDATA%\UnrealEngine\Common\Zen`. During the bounded session, `zenserver.log` continued reporting periodic garbage-collection health lines without error/failed-start messages. That confirms the service was healthy during that session, but does not explain the editor's recurring slow-launch prompt. Capture the editor's own `Saved/Logs` excerpt around `LogZenServer` / `LogDerivedDataCache` before changing Zen lifetime or cache configuration.

## Next check

Once Live Coding is inactive, rebuild the scratch editor target and run `CaptiveSky2.Agent.SocialPacing` plus `CaptiveSky2.Agent.SessionSafety`. In a later bounded play session, confirm the new one-minute gap makes autonomous conversations feel less bursty without starving them.
