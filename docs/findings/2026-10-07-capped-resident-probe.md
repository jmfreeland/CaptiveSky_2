# Capped resident decision probe

On 2026-10-07, a bounded spectator launch was used to diagnose the unavailable editor/runtime and to observe one resident decision without touching the normal world data.

## Launch and isolation

The first launch used `Scripts/Start-Spectator.ps1` with Python disabled, a 180-second play cap, a one-request cap, and a unique `-CaptiveSkyDataRoot`. In the sandboxed launch, Unreal logged an access-denied attempt against its local `EditorSettings.ini` under `%LOCALAPPDATA%`, then stopped making log progress while Turnkey was running `VerifySdk`. No Island world-ready or agent-session marker appeared. Only that launched Game process tree was stopped after more than five minutes; no normal agent or world data had been written.

The retry with elevated access and a separate data root reached the Island in 20.8 seconds. It retained the same 180-second and one-request limits. The run log is `Saved/Logs/Codex_ResidentDecisionProbeElevated_20261007.log`.

## Resident result

`Agent_Aster_01` issued the first decision request, using the full tier with a scene image; the log reports a nearby being at 300 cm and a nearby thing at 597 cm. Reserving that single request ended the bounded session after 14.1 seconds. Code review then showed that the session watchdog treated the reservation count reaching one as immediate expiry, about 90 ms after the HTTP request began. The game shut down with that request still outstanding; its later 30-second timeout log is therefore not evidence that a running game gave the request a full 30 seconds to return. No completed decision, action, or agent memory file was recorded.

The isolated data root contains only `WorldState/chronicle.jsonl` and `WorldState/Island.json`; it has no `Agents/` memory files. The normal `Agents/` and `WorldState/` were not the playtest destination. No additional model request was made.

## What this does and does not show

- The editor was not running when checked. A machine reboot is not indicated by the failed first launch alone: allowing the Unreal process to write its standard AppData files let the Game reach the Island.
- The persistent `dotnet.exe - Application Error` dialog (exception `0xe0434352`) was visible during the checks, but its owning process was not established; do not assume it caused either the Turnkey stall or the API timeout.
- A separate unauthenticated HTTPS GET to `https://api.openai.com/v1/models` returned HTTP 401 in 0.53 seconds (DNS 0.014 seconds; TLS 0.068 seconds), confirming host reachability and TLS from the elevated shell without using the API key or making a model request. This does not establish that the Unreal HTTP stack or authenticated chat-completions call works.
- The request-cap behavior now rejects new calls but drains accepted in-flight calls for at most 45 seconds; the explicit real-time limit still ends play immediately. The UE 5.8.3 editor target build succeeded, and `CaptiveSky2.Agent.SessionSafety` passed with the drain, no-new-request, drain-timeout, and hard-deadline assertions. Log: `Saved/Logs/Codex_SessionSafetyDrain_20261007.log`.
- A follow-up live probe was not launched: tool approval was required because it would transmit Aster's full decision prompt (which can include private memory excerpts) and first-person scene image to `api.openai.com`. No second model request was made.

## Next checks

1. When the editor is needed, launch it with the normal user access needed for Unreal's AppData and build-tool logs, then verify the editor window and MCP connection separately.
2. If explicitly authorized to send the full resident context and scene image to `api.openai.com`, repeat one capped decision probe with the same isolated data-root and request limits. Look for a completed response, chosen action, and isolated memory update before claiming resident agency worked.
