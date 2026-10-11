# Request-Accumulation Playtest — 2026-10-11

## Purpose

Observe a longer, bounded live session to learn which resident requests accumulate, how quickly requests arrive, and whether spontaneous conversations feel too frequent.

## Existing pacing evidence

- `Config/DefaultGame.ini` caps ordinary play at 1,800 real seconds and 120 model requests. The watchdog applies the earlier of the two limits.
- Automatic conversations have a five-minute minimum cooldown **per resident pair**, with a maximum of four spoken lines in one exchange (`AgentSocialComponent`). There is no world-wide conversation-start interval, so different pairs can still take turns talking.
- `Saved/Logs/Claude_PIE.log` records a prior 139-second PIE session with 8 model requests. Raven, Aster, and the Innkeeper each received a decision request near the start; subsequent requests recur at roughly one-minute intervals. Aster opened a conversation with Raven near the end. This suggests a high aggregate decision-request rate, but this short sample does not establish that conversation starts themselves are too frequent.
- `Saved/Logs/LiveMorning_2026-09-28.log` records a 1,500.1-second session with 73 model requests and 19 spoken lines (about 2.9 requests/minute and 0.76 lines/minute). Multi-line exchanges contain turns only 2–3 seconds apart, including a five-line burst in about 9 seconds. This is historical evidence, not a measurement of the current build.
- Current `AgentSocialComponent` queues an incoming line and attempts the reply on its next 0.25-second component tick; it has no intentional real-time pause between conversational turns. The five-minute cooldown only applies after an exchange and only to that pair.

## Reply pacing change

Added a 15-second minimum real-time pause after receiving an automatic in-world line and before requesting the reply. The pause is configurable upward, but old Blueprint values cannot lower it. This spaces the rapid multi-line exchanges seen in the historical log while leaving autonomous thinking, conversation openings, the five-minute per-pair cooldown, the four-line exchange limit, and the session time/request caps unchanged. Each queued reply now logs its participant, turn, conversation ID, and scheduled delay—but not the spoken text—so a bounded live sample can audit the pacing without putting private dialogue into this diagnostic. `AgentSocialPacing` covers the minimum, longer overrides, invalid overrides, and pause expiry. Runtime confirmation remains pending.

## 2026-10-11 launch attempts

Started `Scripts/Start-Spectator.ps1` with an isolated data root, 1280×720 window, Python disabled, a 1,800-second real-time ceiling, a 60-model-request ceiling, and a 300-second startup timeout. The test-specific paths were:

- Log: `Saved/Logs/Codex_RequestAccumulation_20261011.log`
- Data root: `Saved/Playtests/Codex_RequestAccumulation_20261011/World`

The game process never reached the Island world-ready marker. Its last log entries show `LogTurnkeySupport` launching serialized UAT with `-WaitForUATMutex`; no `Intermediate/TurnkeyLog_0.log` was created. After the script's five-minute startup timeout, it terminated only the Game process it had launched. The user's existing editor was not terminated, and the play session never started, so this attempt consumed no model requests and produced no resident-request sample.

This identifies a Turnkey/UAT wait as the immediate startup obstacle for the first attempt; it does **not** prove which existing process owns or blocks the mutex. Do not terminate unrelated editor, `cmd.exe`, or `dotnet.exe` processes based only on this evidence.

A second attempt used the isolated scratch project's `UnrealEditor-Cmd.exe` with `-game -Spectator -NullRHI`, the same 1,800-second / 60-request caps, and the isolated data root. The Island loaded, but the first resident thought crashed while `CaptureFirstPersonSnapshot()` tried to render/export a scene capture under NullRHI (`EXCEPTION_ACCESS_VIOLATION` in `AutonomousAgentCharacter.cpp`). No successful provider request or resident behavior sample was confirmed. The user's open editor was unaffected. A `FApp::CanEverRender()` early return has now been added so non-rendering runs can fall back to text-only agent context; **this follow-up change has not yet been rebuilt or tested**.

At the time of this note, the existing editor was responsive but using about 18 GiB, with roughly 850 MiB of system memory available. The Unreal MCP endpoint on localhost:8000 was also unavailable. Therefore no further Unreal process was started beside the editor. Build/automation verification of the snapshot fallback and a bounded live run remain pending until the editor is closed or materially more memory is available.

Later that session the editor closed and available memory rose to about 18.6 GiB, but a separate `UnrealEditor-Cmd.exe` (PID 39232) appeared, with no readable command line and no matching new project log. It was not started or stopped by this task; do not interfere with it. Hold any new build or playtest until its ownership/session is clear or it exits.

## Next safe observation

The editor and Unreal processes were closed for follow-up. The project Editor target built successfully after the reply-scheduling log was added, and `CaptiveSky2.Agent.SocialPacing` passed in `UnrealEditor-Cmd` with NullRHI, agent thinking disabled, and a zero-model-request cap (`Saved/Logs/Codex_AgentSocialPacing_Log_20261011.log`). `FApp::CanEverRender()` already guards the first-person snapshot path so a later non-rendering run can use text-only context.

A proposed five-minute live observation with a 12-request ceiling was not started: the configured agent backend would receive resident/world context, and that external data transfer was not authorized by the current request. No provider requests were made. If approved, the next sample can use the isolated `Saved/Playtests/Codex_SocialPacing_20261011/World` root, keep spontaneous thinking enabled, and cap at 300 real seconds and 12 model requests. Otherwise, continue with local-only automation and synthetic utterance tests.

## 2026-10-11 bounded live retry

A fresh isolated `UnrealEditor-Cmd` run reached Island in 40 seconds with a 900-second / 30-request cap. The first decision cycle reserved three requests (Raven, Aster, Innkeeper), but all three failed to connect to the configured `https://api.openai.com/v1/chat/completions` endpoint in 0.02 seconds (`libcurl error 7`); no resident decisions or conversations followed. The test process was stopped rather than spending its cap on retries. A separate `Test-NetConnection api.openai.com -Port 443` check also failed from this managed execution environment. A requested network-enabled retry was declined because it would transmit resident prompts and potentially private memory/world context; explicit user approval for that data transfer is still required. No successful provider call was made.

The social component now emits dialogue-free structured log markers for automatic conversation opens, delivered line numbers/participants, and closes. These enable a future authorized live run to distinguish decision-request volume from conversation-start rate without adding speech text to the new diagnostics. Existing conversational behavior and all time/request safeguards remain unchanged. The `CaptiveSky_2Editor` target built successfully. `CaptiveSky2.Agent.SocialPacing` and `CaptiveSky2.Agent.CraftTeaching` passed under NullRHI with thinking disabled and a zero-request cap; the latter exercised the delivered-line markers (`Saved/Logs/Codex_SocialDiagnosticsDelivery_20261011.log`).

For any approved live observation, confirm the new reply pause in the live log and track:

1. model requests by resident and elapsed minute;
2. conversation start/finish timestamps, participants, and line counts;
3. new object/upgrade proposals by kind, status, and resident;
4. memory, world-state, or log errors and the reason the session stops.

Compare the isolated request backlog with the main world. If conversations still feel too frequent, inspect openings across different pairs separately; the per-pair cooldown does not impose a world-wide interval. Preserve the five-minute per-pair minimum and both existing play safeguards.
