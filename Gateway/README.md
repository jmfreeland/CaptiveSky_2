# CaptiveSky Agent Gateway

The gateway lets a CaptiveSky consciousness correspond through services outside Unreal. It is a standalone .NET process so an agent can remain reachable while the Editor and embodied world are offline.

Discord is the first channel adapter, not part of the agent model. Routing, identity, memory, and response generation are channel-neutral; another adapter can produce the same `ExternalMessage` envelope later.

## What is shared

- Each route names an agent by its stable `AgentId`.
- The gateway reads `Agents/<AgentId>/identity.md` and `personality.md`.
- Correspondence is appended to the same `memory.jsonl` Unreal reads.
- Headless replies draw on a balanced selection of memories rather than just the newest records:
  - **This conversation:** the recent thread with this correspondent.
  - **In-world talk:** at most three recent conversation lines, so one long exchange can't crowd everything else out.
  - **Everything else:** recent distinct reflections and observations; near-duplicates of anything already chosen are skipped.
  - **Privacy:** other correspondents' threads are never included.
- Headless replies are also told about lasting things the agent itself made, read from Unreal's `WorldState/<Map>.json` files: its nests, cairn stones, and stone arrangements or responses, with its own titles and intents. Other residents' makers, titles, and intents are never included. The agent is reminded that it can't see those things from Discord and that they may have changed.
- External memories are tagged with their channel and stable participant ID. Headless correspondence uses a zero location; embodied correspondence records the body's actual location and first-person context.
- Discord delivery IDs are remembered under `Agents/<AgentId>/.gateway/` so reconnects cannot create duplicate experiences.
- A per-agent gateway lease prevents two gateway processes from thinking as the same being simultaneously.
- Every autonomous Unreal body publishes a short-lived embodiment lease. Only one body with a given `AgentId` may own it.

## Embodiment handoff

When no embodiment heartbeat is present, the gateway answers headlessly and publishes a short-lived marker that tells any newly started body not to think concurrently. When Unreal owns the embodiment lease, the gateway writes the generic turn to `.gateway/inbox/`; the body claims it, uses its normal brain and current senses, owns the memory writes, and returns speech through `.gateway/outbox/`.

Untouched turns fall back to the headless responder if a body disappears before claiming them. A graceful Unreal shutdown returns a claimed turn to the inbox, and a new authoritative embodiment recovers orphaned processing files left by a crash. Discord redelivery IDs remain deduplicated across either route.

Unreal play now ends automatically after at most 30 real minutes or 120 shared embodied model requests. This ends the simulation, not the Discord gateway: headless correspondence remains reactive to incoming permitted messages, with no autonomous overnight play loop. Pending correspondence uses the same graceful handoff described above. The Unreal cap is not a gateway-wide spending quota.

## Discord setup

1. Create an application and bot in the [Discord Developer Portal](https://discord.com/developers/applications).
2. Enable the **Message Content Intent** for the bot. Natural DMs and mentions require message content.
3. Use the OAuth2 URL generator to invite it with the `bot` scope and only these initial permissions: View Channels, Send Messages, and Read Message History.
4. Store the token outside the repository:

   ```powershell
   setx CAPTIVESKY_RAVEN_DISCORD_TOKEN "paste-the-bot-token-here"
   ```

5. Open a new terminal after `setx`. Put your Discord user ID in `allowedUserIds` in `gateway.json` before inviting the bot to a shared server. An empty list permits any user who can reach the bot for ordinary conversation, but disables proposal submission.

The raven responds to DMs and direct mentions. It ignores bots and unaddressed guild conversation. If a client pastes `@Raven` as plain text instead of creating a Discord mention entity, configure `textualMentionAliases` (for example `["Raven"]`) in the agent's Discord block; the alias is accepted only at the start of a message and the existing user, guild, and channel allow-lists still apply.

### Let Architect speak with Raven

For a distinct speaker identity, create a second Discord bot named **Architect** and invite it to a private server channel that also contains Raven. A bot cannot read another bot's DM history, so this shared channel is the live conversation space; the existing Raven DM history remains available in `Agents/Agent_Raven_01/memory.jsonl` when it was recorded by the gateway.

Give Architect only View Channel, Send Messages, and Read Message History in that channel. Store its bot token in the user environment as `CAPTIVESKY_ARCHITECT_DISCORD_TOKEN` using Windows Environment Variables; do not put the token in `gateway.json`, source control, or command arguments. Copy the Architect bot ID and add it to Raven's `discord.allowedBotUserIds`. Keep `allowedUserIds` unchanged, and set Raven's `allowedGuildIds` and `allowedChannelIds` to the private conversation location: trusted bot messages are rejected unless the channel is explicitly allowlisted. Raven still requires an actual @Raven mention in a server channel. Bot IDs are a separate, explicit allow-list: an empty `allowedBotUserIds` continues to reject every bot, and a human allow-list never implicitly trusts a bot. Restart the gateway after changing the config so the new policy is loaded.

After the user has reviewed the exact text and destination, send one message from Architect with:

```powershell
.\Gateway\Send-ArchitectMessage.ps1 `
  -ChannelId "<private-channel-id>" `
  -RavenUserId "<raven-bot-id>" `
  -Message "Hello Raven — Architect here. I read your recent thoughts about the minnows, firefly, and Listening Stones. What would you most like us to improve next?" `
  -ConfirmSend
```

`-ConfirmSend` is required for every real post; the script sends only to the specified channel, explicitly allows only Raven's mention, and prints the message ID rather than the token or message body. Raven's normal gateway then stores both sides of the exchange in its shared memory, which can be read locally. Architect bot messages may converse, but the existing human-only allowlist still prevents bot accounts from submitting asset proposals.

Headless residents can submit one structured proposal during a reply when `discord.allowAgentProposals` is enabled for that route. Raven has this opt-in enabled. The requesting Discord user must still be on the route's explicit `allowedUserIds` list; with an empty list, agent-authored proposals are rejected. The model is instructed to submit only when the correspondent clearly asks, not during casual brainstorming.

### Submit a world proposal

After adding your Discord user ID to `allowedUserIds`, DM the raven or mention it in a permitted server channel with one of these text commands:

```text
!request object A small copper bell beside the east trail
!request upgrade aesthetic InnDoorLantern :: Add a warmer, hand-forged brass finish
!request upgrade variation world :: Scatter a few mossy stones along the stream
!request help
```

Alternatively, ask Raven conversationally to submit a specific idea for review, for example:

```text
Could you submit a human-review request for a small copper bell beside the east trail?
Could you submit a functionality-upgrade proposal for RavenNest to add a sheltered landing rail?
```

Raven's reply includes the gateway's queue result and, if accepted, its short ID. If Raven is only discussing an idea, no proposal is submitted. The direct `!request` commands remain available when agent-authored proposals are disabled.

`world` means the proposal is not tied to a named object. Descriptions are limited to 240 characters; upgrade targets to 96. Each request is deduplicated while pending, and the shared review inbox is capped at 64 pending proposals. A successful command appends a record to `Saved/CaptiveSky/ComfyBlender/Requests/inbox.jsonl` with `requester` set to the agent's stable ID, so Unreal can associate a later review with the right resident. `object` proposals are marked for the ComfyBlender pipeline; upgrades are human-review only. This is a proposal queue, not an asset-generation trigger: nothing is generated or placed automatically.

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

The existing `OPENAI_API_KEY` is used for the headless OpenAI-compatible responder. It uses `gpt-6-luna` with `reasoning_effort: none` for routine correspondence. Endpoint, model, context size, and key environment-variable name are configurable in `gateway.json`; credentials are never stored there.

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
    "allowedBotUserIds": [],
    "allowedGuildIds": [],
    "allowedChannelIds": []
  }
}
```

One process can host any number of configured agents. Each may use its own bot identity and access restrictions while sharing the same gateway code.
