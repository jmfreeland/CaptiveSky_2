# Capped resident decision probe

On 2026-10-07, a bounded spectator launch was used to diagnose the unavailable editor/runtime and to observe one resident decision without touching the normal world data.

## Launch and isolation

The first launch used `Scripts/Start-Spectator.ps1` with Python disabled, a 180-second play cap, a one-request cap, and a unique `-CaptiveSkyDataRoot`. In the sandboxed launch, Unreal logged an access-denied attempt against its local `EditorSettings.ini` under `%LOCALAPPDATA%`, then stopped making log progress while Turnkey was running `VerifySdk`. No Island world-ready or agent-session marker appeared. Only that launched Game process tree was stopped after more than five minutes; no normal agent or world data had been written.

The retry with elevated access and a separate data root reached the Island in 20.8 seconds. It retained the same 180-second and one-request limits. The run log is `Saved/Logs/Codex_ResidentDecisionProbeElevated_20261007.log`.

## Resident result

`Agent_Aster_01` issued the first decision request, using the full tier with a scene image; the log reports a nearby being at 300 cm and a nearby thing at 597 cm. Reserving that single request immediately ended the bounded session after 14.1 seconds, as intended. The POST to `https://api.openai.com/v1/chat/completions` then timed out after 30 seconds. Shutdown logged one still-outstanding HTTP request. No completed decision, action, or agent memory file was recorded.

The isolated data root contains only `WorldState/chronicle.jsonl` and `WorldState/Island.json`; it has no `Agents/` memory files. The normal `Agents/` and `WorldState/` were not the playtest destination. No additional model request was made.

## What this does and does not show

- The editor was not running when checked. A machine reboot is not indicated by the failed first launch alone: allowing the Unreal process to write its standard AppData files let the Game reach the Island.
- The persistent `dotnet.exe - Application Error` dialog (exception `0xe0434352`) was visible during the checks, but its owning process was not established; do not assume it caused either the Turnkey stall or the API timeout.
- The runtime request timeout is a separate unresolved issue. The log proves a single request timed out, not whether the cause was network reachability, TLS/certificates, or service response. Do not increase the request cap or repeat the live call until that path is understood.

## Next checks

1. When the editor is needed, launch it with the normal user access needed for Unreal's AppData and build-tool logs, then verify the editor window and MCP connection separately.
2. Diagnose the UE process's HTTPS connectivity and certificate/proxy environment without sending another model request. Keep the same isolated data-root and request caps for any later behavioral probe.
3. After connectivity is verified, repeat one capped decision probe and look for a completed response, chosen action, and isolated memory update before claiming resident agency worked.
