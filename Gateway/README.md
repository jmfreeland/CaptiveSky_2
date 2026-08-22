# CaptiveSky Agent Gateway

The gateway lets a CaptiveSky consciousness correspond through services outside Unreal. It is a standalone .NET process so an agent can remain reachable while the Editor and embodied world are offline.

Discord is the first channel adapter, not part of the agent model. Routing, identity, memory, and response generation are channel-neutral; another adapter can produce the same `ExternalMessage` envelope later.

## What is shared

- Each route names an agent by its stable `AgentId`.
- The gateway reads `Agents/<AgentId>/identity.md` and `personality.md`.
- Correspondence is appended to the same `memory.jsonl` Unreal reads.
- External memories are tagged with their channel and stable participant ID. Headless correspondence uses a zero location; embodied correspondence records the body's actual location and first-person context.
- Discord delivery IDs are remembered under `Agents/<AgentId>/.gateway/` so reconnects cannot create duplicate experiences.
- A per-agent gateway lease prevents two gateway processes from thinking as the same being simultaneously.
- Every autonomous Unreal body publishes a short-lived embodiment lease. Only one body with a given `AgentId` may own it.

## Embodiment handoff

When no embodiment heartbeat is present, the gateway answers headlessly and publishes a short-lived marker that tells any newly started body not to think concurrently. When Unreal owns the embodiment lease, the gateway writes the generic turn to `.gateway/inbox/`; the body claims it, uses its normal brain and current senses, owns the memory writes, and returns speech through `.gateway/outbox/`.

Untouched turns fall back to the headless responder if a body disappears before claiming them. A graceful Unreal shutdown returns a claimed turn to the inbox, and a new authoritative embodiment recovers orphaned processing files left by a crash. Discord redelivery IDs remain deduplicated across either route.

## Discord setup

1. Create an application and bot in the [Discord Developer Portal](https://discord.com/developers/applications).
2. Enable the **Message Content Intent** for the bot. Natural DMs and mentions require message content.
3. Use the OAuth2 URL generator to invite it with the `bot` scope and only these initial permissions: View Channels, Send Messages, and Read Message History.
4. Store the token outside the repository:

   ```powershell
   setx CAPTIVESKY_RAVEN_DISCORD_TOKEN "paste-the-bot-token-here"
   ```

5. Open a new terminal after `setx`. Put your Discord user ID in `allowedUserIds` in `gateway.json` before inviting the bot to a shared server. An empty list permits any user who can reach the bot.

The raven responds to DMs and direct mentions. It ignores bots and unaddressed guild conversation.

## Run locally

```powershell
dotnet restore Gateway/src/CaptiveSky.Gateway/CaptiveSky.Gateway.csproj --configfile Gateway/NuGet.Config
dotnet run --project Gateway/src/CaptiveSky.Gateway/CaptiveSky.Gateway.csproj -- --check Gateway/gateway.json
dotnet run --project Gateway/src/CaptiveSky.Gateway/CaptiveSky.Gateway.csproj -- --ready Gateway/gateway.json
dotnet run --project Gateway/src/CaptiveSky.Gateway/CaptiveSky.Gateway.csproj -- Gateway/gateway.json
```

`--check` validates configuration and agent homes while reporting missing secrets without failing. `--ready` additionally requires every enabled Discord token and, when `llm.requireApiKey` is true, the configured LLM key; it exits with code 2 when activation is incomplete.

For an unattended process that restarts after failures and writes daily logs under `Gateway/logs/`:

```powershell
Gateway/start-gateway.ps1
```

After the readiness check passes, an optional per-user scheduled task can launch that wrapper whenever you log on:

```powershell
Gateway/install-startup-task.ps1
Start-ScheduledTask -TaskName 'CaptiveSky Agent Gateway'
```

The installer is deliberately not run automatically. It creates only the task definition; secrets remain environment variables and are never copied into the task or logs. This logon task is suitable for a continuously logged-in workstation. A future server deployment can run the same gateway executable under a real service manager without changing its routing or agent model.

The existing `OPENAI_API_KEY` is used for the headless OpenAI-compatible responder. Endpoint, model, context size, and key environment-variable name are configurable in `gateway.json`; credentials are never stored there.

Run the integration checks with:

```powershell
dotnet run --project Gateway/tests/CaptiveSky.Gateway.Checks/CaptiveSky.Gateway.Checks.csproj
```

## Add another agent

Create its normal `Agents/<AgentId>/` home, create a separate Discord application when it should have its own visible identity, and add another entry to `gateway.json`:

```json
{
  "agentId": "Agent_Aster_01",
  "enabled": true,
  "discord": {
    "enabled": true,
    "tokenEnvironmentVariable": "CAPTIVESKY_ASTER_DISCORD_TOKEN",
    "allowDirectMessages": true,
    "respondToMentions": true,
    "allowedUserIds": [123456789012345678],
    "allowedGuildIds": [],
    "allowedChannelIds": []
  }
}
```

One process can host any number of configured agents. Each may use its own bot identity and access restrictions while sharing the same gateway code.
