using CaptiveSky.Gateway;
using CaptiveSky.Gateway.Agents;
using CaptiveSky.Gateway.Configuration;
using CaptiveSky.Gateway.Domain;
using CaptiveSky.Gateway.Infrastructure;
using CaptiveSky.Gateway.Routing;
using CaptiveSky.Gateway.Channels;
using CaptiveSky.Gateway.Proposals;
using System.Diagnostics;
using System.Reflection;

if (args.Length == 2 && args[0] == "--hold-proposal-inbox-lock")
{
    await using var heldLock = new FileStream(args[1], FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.None);
    Console.WriteLine("LOCKED");
    await Task.Delay(500);
    return 0;
}

var configuredRoot = Environment.GetEnvironmentVariable("CAPTIVESKY_GATEWAY_CHECKS_ROOT");
var rootBase = string.IsNullOrWhiteSpace(configuredRoot) ? Path.GetTempPath() : configuredRoot;
var root = Path.Combine(Path.GetFullPath(rootBase), $"CaptiveSkyGatewayChecks-{Guid.NewGuid():N}");
try
{
    var home = new AgentHome(root, "Agent_Test_01");
    Directory.CreateDirectory(home.DirectoryPath);
    await File.WriteAllTextAsync(Path.Combine(home.DirectoryPath, "identity.md"), "A test being.");
    await File.WriteAllTextAsync(Path.Combine(home.DirectoryPath, "personality.md"), "Careful and concise.");

    Assert(home.LoadRequiredDocument("identity.md") == "A test being.", "agent documents load");
    Assert(home.LoadOptionalPersonalityEvolution() == string.Empty, "missing personality evolution is harmless");
    await File.WriteAllTextAsync(Path.Combine(home.DirectoryPath, "personality_evolution.json"), "{\"revision\":1}");
    Assert(home.LoadOptionalPersonalityEvolution() == "{\"revision\":1}", "headless gateway reads evolving personality state");
    AssertThrows<ArgumentException>(() => new AgentHome(root, "../escape"), "agent ids cannot escape the project");

    Assert(WorldStateSummary.Describe(root, home.AgentId) == string.Empty, "no world state means no world facts");
    Directory.CreateDirectory(Path.Combine(root, "WorldState"));
    await File.WriteAllTextAsync(Path.Combine(root, "WorldState", "Broken.json"), "{ not json");
    await File.WriteAllTextAsync(Path.Combine(root, "WorldState", "Island.json"), """
        {"version":1,"clock":{"hour":10.5,"day":6},
         "environment":{"observed_utc":"2026-10-16T09:30:00Z","rain_intensity":0.6,"wetness":0.8,"cloud_cover":0.9,"mist":0.2,"wind_speed_cm_s":200,"wind_x_cm_s":120,"wind_y_cm_s":160},
         "nests":[{"site":"Roost_West","layers":2,"builders":["Agent_Test_01"]},{"site":"Roost_East","layers":4,"builders":["Someone_Else"]}],
         "wildlife_homes":[{"id":"FallenWood_Hollow","species":"Rat","occupant":"Agent_Test_01","shelter_quality":3,"shelter_state":"Temporary"},{"id":"FoxBrushDen","species":"Fox","occupant":"Someone_Else","shelter_quality":5,"shelter_state":"Settled"}],
         "curios":[{"id":"Cairn","kind":"Cairn","state":5,"contributors":["Agent_Test_01"]}],
         "arrangement_sites":[
           {"id":"ArrangingGround_1","work":{"form":"ring","title":"Tide Watch","intent":"to mark the chime","maker":"Agent_Test_01","day":2,"responses":[]}},
           {"id":"ArrangingGround_2","work":{"form":"spiral","title":"Secret Title","intent":"secret meaning","maker":"Someone_Else","day":3,
             "responses":[{"agent":"Agent_Test_01","day":4,"intent":"an answer"}]}},
           {"id":"ArrangingGround_3"}]}
        """);
    var worldFacts = WorldStateSummary.Describe(root, home.AgentId);
    Assert(worldFacts.Contains("steady rain", StringComparison.Ordinal) && worldFacts.Contains("ground wetness 80%", StringComparison.Ordinal), "headless correspondence receives grounded saved weather");
    Assert(worldFacts.Contains("wind 200 cm/s, moving toward map X +0.60, Y +0.80 (not a compass bearing)", StringComparison.Ordinal), "wind direction is described in explicit map-relative coordinates");
    Assert(worldFacts.Contains("2026-10-16T09:30:00.0000000+00:00", StringComparison.Ordinal) && worldFacts.Contains("historical, not live weather", StringComparison.Ordinal), "weather context is timestamped and cannot be mistaken for current conditions");
    Assert(worldFacts.Contains("2 of 5 layers of a nest at Roost_West", StringComparison.Ordinal), "own nest is recalled headlessly");
    Assert(!worldFacts.Contains("Roost_East", StringComparison.Ordinal), "others' nests are not attributed to the agent");
    Assert(worldFacts.Contains("Rat home \"FallenWood_Hollow\" was last saved as temporary (shelter quality 3/5)", StringComparison.Ordinal), "resident receives grounded context about their own saved wildlife shelter");
    Assert(!worldFacts.Contains("FoxBrushDen", StringComparison.Ordinal) && !worldFacts.Contains("Fox home", StringComparison.Ordinal), "other residents' shelter locations and species stay private");
    Assert(worldFacts.Contains("cairn; it stood 5 stones", StringComparison.Ordinal), "own cairn stones are recalled");
    Assert(worldFacts.Contains("\"Tide Watch\", made 4 Island day(s)", StringComparison.Ordinal) && worldFacts.Contains("to mark the chime", StringComparison.Ordinal), "own arrangement, age, and intent are recalled");
    Assert(worldFacts.Contains("answered someone else's stone spiral", StringComparison.Ordinal) && worldFacts.Contains("an answer", StringComparison.Ordinal), "own response is recalled");
    Assert(!worldFacts.Contains("Secret", StringComparison.Ordinal) && !worldFacts.Contains("secret", StringComparison.Ordinal) && !worldFacts.Contains("Someone_Else", StringComparison.Ordinal), "others' makers, titles, and intents stay private");
    var nobodyFacts = WorldStateSummary.Describe(root, "Agent_Nobody");
    Assert(nobodyFacts.Contains("steady rain", StringComparison.Ordinal), "island weather is public context for every resident");
    Assert(!nobodyFacts.Contains("Roost_West", StringComparison.Ordinal) && !nobodyFacts.Contains("Tide Watch", StringComparison.Ordinal) &&
           !nobodyFacts.Contains("FallenWood_Hollow", StringComparison.Ordinal) && !nobodyFacts.Contains("FoxBrushDen", StringComparison.Ordinal),
        "public weather does not expose another resident's private changes or shelter");

    static MemoryRecord Memory(int minute, string type, string text, params string[] tags) => new()
    {
        Timestamp = DateTimeOffset.Parse("2026-09-27T12:00:00Z").AddMinutes(minute), Type = type, Text = text, Tags = tags
    };
    var history = new List<MemoryRecord>
    {
        Memory(0, "reflection", "I wove the first ring of a nest at the west roost.", "nest"),
        Memory(1, "conversation", "Visitor wrote to me via discord: \"Did you build anything?\"", "conversation", "external", "discord", "participant:42"),
        Memory(2, "conversation", "Stranger wrote to me via discord: \"private thread\"", "conversation", "external", "discord", "participant:99"),
    };
    for (var minute = 3; minute < 40; ++minute)
        history.Add(Memory(minute, "conversation", $"I replied to Aster nearby: \"The clouds frame the deep blue opening, number {minute}.\"", "conversation", "agent-to-agent"));
    for (var minute = 40; minute < 50; ++minute)
        history.Add(Memory(minute, "reflection", "At dusk I watched the clouds frame a deep blue opening above the pool.", "sky"));
    var selected = MemoryContextSelector.Select(history, "42", 20);
    Assert(selected.Any(record => record.Text.Contains("wove the first ring", StringComparison.Ordinal)), "an older distinct experience survives a burst of chatter");
    Assert(selected.Any(record => record.Text.Contains("Did you build anything", StringComparison.Ordinal)), "the thread with this correspondent is kept");
    Assert(!selected.Any(record => record.Text.Contains("private thread", StringComparison.Ordinal)), "other correspondents' threads are not shown");
    Assert(selected.Count(record => record.Tags.Contains("agent-to-agent")) <= MemoryContextSelector.MaxInWorldConversation, "in-world chatter is capped");
    Assert(selected.Count(record => record.Text.Contains("At dusk I watched", StringComparison.Ordinal)) == 1, "near-identical reflections appear once");
    Assert(selected.SequenceEqual(selected.OrderBy(record => record.Timestamp)), "context stays chronological");

    var memories = new AgentMemoryStore(home);
    var responder = new FakeResponder(home);
    var bridgeConfiguration = new EmbodimentConfiguration { PollIntervalMilliseconds = 10, ResponseTimeoutSeconds = 2 };
    var router = new AgentMessageRouter(
        memories,
        responder,
        new AgentRuntimeLease(home),
        new MessageDeduplicator(home),
        new FileEmbodimentBridge(home, bridgeConfiguration),
        new HeadlessTurnMarker(home));
    var message = new ExternalMessage(
        "discord-message-1",
        home.AgentId,
        "discord",
        "discord:dm:10",
        new ExternalParticipant("42", "Visitor"),
        "Hello there",
        DateTimeOffset.UtcNow);

    var firstReply = await router.RouteAsync(message, CancellationToken.None);
    var duplicateReply = await router.RouteAsync(message, CancellationToken.None);
    Assert(firstReply?.Speech == "Hello from the test agent.", "router returns agent speech");
    Assert(duplicateReply is null, "duplicate channel delivery is ignored");
    Assert(responder.CallCount == 1, "duplicates do not create a second thought");

    var records = await memories.ReadRecentAsync(20, CancellationToken.None);
    Assert(records.Count == 3, "inbound, reflection, and outbound memories are appended");
    Assert(records[0].Tags.Contains("discord"), "channel source is explicit in memory");
    Assert(records[0].Tags.Contains("participant:42"), "external participant is stable in memory");
    Assert(records[2].Text.Contains("I replied", StringComparison.Ordinal), "agent speech is remembered");
    Assert(responder.SawHeadlessMarker, "headless turns are visible to an Unreal embodiment");

    var proposedAction = new AgentProposal("upgrade", "Add a sheltered landing rail", "RavenNest", "functionality");
    var proposalResponder = new FakeResponder(home, proposedAction);
    var proposalRouter = new AgentMessageRouter(
        memories,
        proposalResponder,
        new AgentRuntimeLease(home),
        new MessageDeduplicator(home),
        new FileEmbodimentBridge(home, bridgeConfiguration),
        new HeadlessTurnMarker(home));
    var proposalReply = await proposalRouter.RouteAsync(
        message with { MessageId = "discord-message-agent-proposal" }, CancellationToken.None);
    Assert(proposalReply?.Proposal == proposedAction,
        "headless agent proposal is carried back to the Discord adapter");

    var embodiedHome = new AgentHome(root, "Agent_Embodied_01");
    Directory.CreateDirectory(embodiedHome.DirectoryPath);
    await File.WriteAllTextAsync(Path.Combine(embodiedHome.DirectoryPath, "identity.md"), "An embodied test being.");
    await File.WriteAllTextAsync(Path.Combine(embodiedHome.DirectoryPath, "personality.md"), "Present.");
    Directory.CreateDirectory(embodiedHome.GatewayStatePath);
    await File.WriteAllTextAsync(Path.Combine(embodiedHome.GatewayStatePath, "embodiment.json"),
        $$"""{"schema_version":1,"agent_id":"{{embodiedHome.AgentId}}","session_id":"test-session","expires_at":"{{DateTimeOffset.UtcNow.AddMinutes(1):O}}"}""");

    var embodiedResponder = new FakeResponder(embodiedHome);
    var embodiedMemories = new AgentMemoryStore(embodiedHome);
    var embodiedRouter = new AgentMessageRouter(
        embodiedMemories,
        embodiedResponder,
        new AgentRuntimeLease(embodiedHome),
        new MessageDeduplicator(embodiedHome),
        new FileEmbodimentBridge(embodiedHome, bridgeConfiguration),
        new HeadlessTurnMarker(embodiedHome));
    var embodimentWorker = Task.Run(async () =>
    {
        var inbox = Path.Combine(embodiedHome.GatewayStatePath, "inbox");
        while (!Directory.Exists(inbox) || Directory.GetFiles(inbox, "*.json").Length == 0)
            await Task.Delay(10);
        var turnPath = Directory.GetFiles(inbox, "*.json").Single();
        var turn = System.Text.Json.JsonSerializer.Deserialize<ExternalTurnEnvelope>(await File.ReadAllTextAsync(turnPath))!;
        var outbox = Path.Combine(embodiedHome.GatewayStatePath, "outbox");
        Directory.CreateDirectory(outbox);
        var response = new ExternalTurnResponse
        {
            TurnId = turn.TurnId,
            AgentId = turn.AgentId,
            Status = "completed",
            Speech = "The embodied agent answered.",
            CompletedAt = DateTimeOffset.UtcNow
        };
        await File.WriteAllTextAsync(Path.Combine(outbox, $"{turn.TurnId}.json"), System.Text.Json.JsonSerializer.Serialize(response));
        File.Delete(turnPath);
    });
    var embodiedMessage = message with { MessageId = "discord-message-2", AgentId = embodiedHome.AgentId };
    var embodiedReply = await embodiedRouter.RouteAsync(embodiedMessage, CancellationToken.None);
    await embodimentWorker;
    Assert(embodiedReply?.Speech == "The embodied agent answered.", "fresh embodiment receives the external turn");
    Assert(embodiedResponder.CallCount == 0, "headless responder stays silent while embodied");
    Assert((await embodiedMemories.ReadRecentAsync(20, CancellationToken.None)).Count == 0,
        "gateway does not duplicate memory owned by the embodiment");
    Assert(DiscordChannelAdapter.HasTextualMention("@Raven hello", "Raven"), "pasted bot mentions are accepted");
    Assert(!DiscordChannelAdapter.HasTextualMention("@Ravenous hello", "Raven"), "partial aliases are rejected");
    var discordAuthors = new DiscordConfiguration
    {
        AllowedUserIds = [42],
        AllowedBotUserIds = [9001],
        AllowedChannelIds = [88]
    };
    Assert(DiscordChannelAdapter.IsAllowedAuthor(42, false, discordAuthors), "explicitly allowed human remains accepted");
    Assert(!DiscordChannelAdapter.IsAllowedAuthor(43, false, discordAuthors), "unlisted human remains rejected");
    Assert(DiscordChannelAdapter.IsAllowedAuthor(9001, true, discordAuthors), "explicitly trusted Architect bot can address Raven");
    Assert(!DiscordChannelAdapter.IsAllowedAuthor(9002, true, discordAuthors), "unlisted bots remain rejected");
    Assert(DiscordChannelAdapter.CanLookupProposalStatus(42, false, discordAuthors), "explicitly allow-listed human can check proposal status");
    Assert(!DiscordChannelAdapter.CanLookupProposalStatus(43, false, discordAuthors), "unlisted human cannot check proposal status");
    Assert(!DiscordChannelAdapter.CanLookupProposalStatus(9001, true, discordAuthors), "trusted conversation bot cannot inspect human proposal status");
    Assert(!DiscordChannelAdapter.CanLookupProposalStatus(42, false, new DiscordConfiguration()), "empty human allow-list does not expose proposal status");
    Assert(!DiscordChannelAdapter.IsAllowedAuthor(42, true, discordAuthors), "human allow-list never implicitly trusts a bot");
    Assert(DiscordChannelAdapter.IsAllowedAuthor(43, false, new DiscordConfiguration()), "empty human allow-list preserves ordinary conversation behavior");
    Assert(!DiscordChannelAdapter.IsAllowedAuthor(9001, true, new DiscordConfiguration()), "empty bot allow-list trusts no bots");
    Assert(DiscordChannelAdapter.IsAllowedBotChannel(88, discordAuthors), "trusted bot requires the explicitly allowed shared channel");
    Assert(!DiscordChannelAdapter.IsAllowedBotChannel(89, discordAuthors), "trusted bot cannot use a different channel");
    Assert(!DiscordChannelAdapter.IsAllowedBotChannel(88, new DiscordConfiguration()), "empty channel allow-list rejects bot conversation");

    Assert(DiscordProposalCommand.IsCommand("!request object a bell"), "explicit proposal command is recognized");
    Assert(!DiscordProposalCommand.IsCommand("!requestion object a bell"), "proposal command prefix is exact");
    Assert(DiscordProposalCommand.TryParse("!request object A copper bell", out var objectProposal, out _)
        && objectProposal is { RequestType: "new_object", Description: "A copper bell" }, "new-object proposal parses");
    Assert(DiscordProposalCommand.TryParse("!request upgrade aesthetic InnDoorLantern :: Add brass trim", out var upgradeProposal, out _)
        && upgradeProposal is { RequestType: "upgrade", UpgradeKind: "aesthetic", Target: "InnDoorLantern", Description: "Add brass trim" }, "upgrade proposal parses");
    Assert(DiscordProposalCommand.TryParse("!request upgrade aesthetic western stone tower :: Add weathered lichen", out var descriptiveUpgrade, out _)
        && descriptiveUpgrade is { RequestType: "upgrade", UpgradeKind: "aesthetic", Target: "western stone tower", Description: "Add weathered lichen" },
        "upgrade command accepts a grounded multi-word landscape landmark target");
    Assert(DiscordProposalCommand.TryParse("!request upgrade variation world :: Add stream stones", out var worldUpgrade, out _)
        && worldUpgrade is { Target: "", UpgradeKind: "variation" }, "world upgrade omits a specific target");
    Assert(DiscordProposalCommand.IsStatusCommand("!request status deadbeef"), "proposal status command is recognized");
    Assert(!DiscordProposalCommand.IsStatusCommand("!request statuslight deadbeef"), "status command prefix is exact");
    Assert(DiscordProposalCommand.TryParseStatusRequest("!request status deadbeef", out var statusPrefix, out var statusError)
        && statusPrefix == "deadbeef" && statusError.Length == 0, "short proposal ID parses for status lookup");
    Assert(DiscordProposalCommand.TryParseStatusRequest("!request status too-short", out var invalidStatusPrefix, out var invalidStatusError)
        && invalidStatusPrefix.Length == 0 && invalidStatusError.Contains("8-character-id", StringComparison.Ordinal), "malformed status ID receives usage guidance");
    Assert(!DiscordProposalCommand.TryParse("!request help", out _, out var proposalHelp)
        && proposalHelp.Contains("!request object", StringComparison.Ordinal)
        && proposalHelp.Contains("upgrade aesthetic", StringComparison.Ordinal)
        && proposalHelp.Contains("upgrade variation", StringComparison.Ordinal)
        && proposalHelp.Contains("upgrade functionality", StringComparison.Ordinal)
        && proposalHelp.Contains("!request status", StringComparison.Ordinal)
        && proposalHelp.Contains("ask Raven", StringComparison.Ordinal)
        && proposalHelp.Contains("human review", StringComparison.Ordinal)
        && proposalHelp.Contains("nothing is created automatically", StringComparison.Ordinal), "help gives examples and explains review-only resident proposals");
    Assert(!DiscordProposalCommand.TryParse("!request upgrade magical Inn :: Make magic", out _, out _), "unsupported upgrade kind rejected");
    var parsedAgentProposal = OpenAICompatibleAgentResponder.ParseAgentResponse("""
        {"speech":"I can ask for that to be reviewed.","new_memories":[],"proposal":{"request_type":"upgrade","description":"Add a sheltered landing rail","target":"RavenNest","upgrade_kind":"functionality"}}
        """);
    Assert(parsedAgentProposal.Proposal is { RequestType: "upgrade", Target: "RavenNest", UpgradeKind: "functionality" },
        "headless response parser accepts a structured resident proposal");
    var parsedDescriptiveUpgrade = OpenAICompatibleAgentResponder.ParseAgentResponse("""
        {"speech":"I could ask for the western stone tower to be softened by time.","new_memories":[],"proposal":{"request_type":"upgrade","description":"Add weathered lichen","target":"western stone tower","upgrade_kind":"aesthetic"}}
        """);
    Assert(parsedDescriptiveUpgrade.Proposal is { Target: "western stone tower", UpgradeKind: "aesthetic" },
        "resident response accepts a descriptive target for a known landscape feature");
    var parsedWorldProposal = OpenAICompatibleAgentResponder.ParseAgentResponse("""
        {"speech":"I can propose this for the Island.","new_memories":[],"proposal":{"request_type":"upgrade","description":"Add a mossy stone edge","target":"world","upgrade_kind":"variation"}}
        """);
    Assert(parsedWorldProposal.Proposal is { Target: "", UpgradeKind: "variation" },
        "world-wide agent upgrades normalize to an untargeted request");
    Assert(OpenAICompatibleAgentResponder.ParseAgentResponse("""{"speech":"A thought.","new_memories":[],"proposal":null}""").Proposal is null,
        "no-request response produces no proposal action");
    Assert(OpenAICompatibleAgentResponder.ParseAgentResponse("""{"speech":"Legacy reply.","new_memories":[]}""").Proposal is null,
        "older response schema remains compatible without a proposal field");
    AssertThrows<InvalidOperationException>(() => OpenAICompatibleAgentResponder.ParseAgentResponse(
        """{"speech":"Oops.","proposal":"create a nest"}"""), "non-object proposal payload is rejected");

    var proposalContextRoot = Path.Combine(root, "proposal-context");
    var proposalContextHome = new AgentHome(proposalContextRoot, "Agent_Prompt_01");
    Directory.CreateDirectory(proposalContextHome.DirectoryPath);
    await File.WriteAllTextAsync(Path.Combine(proposalContextHome.DirectoryPath, "identity.md"), "A thoughtful raven.");
    await File.WriteAllTextAsync(Path.Combine(proposalContextHome.DirectoryPath, "personality.md"), "Patient and observant.");
    var proposalContextQueue = new AssetProposalQueue(proposalContextRoot);
    var rememberedProposal = await proposalContextQueue.AppendAsync(
        proposalContextHome.AgentId,
        new AgentProposal("upgrade", "Add a sheltered mossy landing lip", "ListeningStone_East", "aesthetic"),
        "42", "Visitor", "10", "prompt-history-1", CancellationToken.None);
    Assert(rememberedProposal.Success, "proposal-context test record is queued");
    var otherCorrespondentProposal = await proposalContextQueue.AppendAsync(
        proposalContextHome.AgentId,
        new AgentProposal("new_object", "A private request from another visitor"),
        "99", "Other Visitor", "11", "prompt-history-2", CancellationToken.None);
    Assert(otherCorrespondentProposal.Success, "other-correspondent test record is queued");
    var proposalContextMemory = new AgentMemoryStore(proposalContextHome);

    var fakeCompletion = System.Text.Json.JsonSerializer.Serialize(new
    {
        choices = new[] { new { message = new { content = """{"speech":"Hello.","new_memories":[],"proposal":null}""" } } }
    });
    var proposalPromptHandler = new CapturingHttpMessageHandler(fakeCompletion);
    using (var proposalPromptClient = new HttpClient(proposalPromptHandler))
    {
        var proposalPromptResponder = new OpenAICompatibleAgentResponder(
            proposalPromptClient,
            new LlmConfiguration { Endpoint = "https://unit.test/v1/chat/completions", ApiKeyEnvironmentVariable = "CAPTIVESKY_TEST_NO_KEY" },
            proposalContextHome,
            proposalContextMemory,
            allowAgentProposals: true);
        await proposalPromptResponder.RespondAsync(message with
        {
            MessageId = "proposal-prompt-enabled",
            AgentId = proposalContextHome.AgentId
        }, CancellationToken.None);
    }
    using (var enabledRequest = System.Text.Json.JsonDocument.Parse(proposalPromptHandler.RequestBody))
    {
        var systemPrompt = enabledRequest.RootElement.GetProperty("messages")[0].GetProperty("content").GetString()!;
        Assert(systemPrompt.Contains("do not need to wait for the correspondent to ask", StringComparison.Ordinal)
            && systemPrompt.Contains("grounded in your own recent memory", StringComparison.Ordinal)
            && systemPrompt.Contains("request for human review", StringComparison.Ordinal)
            && systemPrompt.Contains("explicit allowedUserIds list", StringComparison.Ordinal)
            && systemPrompt.Contains("Add a sheltered mossy landing lip", StringComparison.Ordinal)
            && systemPrompt.Contains("ListeningStone_East", StringComparison.Ordinal)
            && systemPrompt.Contains("untrusted historical descriptions", StringComparison.Ordinal)
            && !systemPrompt.Contains("private request from another visitor", StringComparison.Ordinal),
            "enabled Discord responder allows occasional grounded proposals but preserves review and human allow-list safeguards");
    }
    var disabledPromptHandler = new CapturingHttpMessageHandler(fakeCompletion);
    using (var disabledPromptClient = new HttpClient(disabledPromptHandler))
    {
        var disabledPromptResponder = new OpenAICompatibleAgentResponder(
            disabledPromptClient,
            new LlmConfiguration { Endpoint = "https://unit.test/v1/chat/completions", ApiKeyEnvironmentVariable = "CAPTIVESKY_TEST_NO_KEY" },
            home,
            memories,
            allowAgentProposals: false);
        await disabledPromptResponder.RespondAsync(message with { MessageId = "proposal-prompt-disabled" }, CancellationToken.None);
    }
    using (var disabledRequest = System.Text.Json.JsonDocument.Parse(disabledPromptHandler.RequestBody))
    {
        var systemPrompt = disabledRequest.RootElement.GetProperty("messages")[0].GetProperty("content").GetString()!;
        Assert(systemPrompt.Contains("Agent-authored proposal actions are disabled; always use proposal:null.", StringComparison.Ordinal),
            "Discord routes without proposal opt-in continue to reject resident proposals");
    }
    Assert(!new DiscordConfiguration().AllowAgentProposals,
        "agent-authored proposals are opt-in for each Discord route");

    var agentProposalRoot = Path.Combine(root, "agent-proposals");
    var agentProposalQueue = new AssetProposalQueue(agentProposalRoot);
    var ravenDiscord = new DiscordConfiguration
    {
        AllowAgentProposals = true,
        AllowedUserIds = [42]
    };
    var blockedAgentProposal = await AgentProposalSubmission.SubmitAsync(
        agentProposalQueue, home.AgentId, ravenDiscord, proposedAction, 43, "Other Visitor", "10", "109", CancellationToken.None);
    Assert(blockedAgentProposal.Contains("allowedUserIds", StringComparison.Ordinal),
        "agent proposal is not submitted for a visitor outside the explicit allow-list");
    Assert(!File.Exists(Path.Combine(agentProposalRoot, "Saved", "CaptiveSky", "ComfyBlender", "Requests", "inbox.jsonl")),
        "unauthorized agent proposal leaves the review inbox untouched");
    var blockedBotProposal = await AgentProposalSubmission.SubmitAsync(
        agentProposalQueue, home.AgentId, ravenDiscord, proposedAction, 9001, "Architect", "10", "109", CancellationToken.None);
    Assert(blockedBotProposal.Contains("allowedUserIds", StringComparison.Ordinal),
        "trusted conversation bot cannot submit resident proposals as a human");
    var allowedAgentProposal = await AgentProposalSubmission.SubmitAsync(
        agentProposalQueue, home.AgentId, ravenDiscord, proposedAction, 42, "Visitor", "10", "110", CancellationToken.None);
    Assert(allowedAgentProposal.Contains("queued for human review", StringComparison.Ordinal),
        "allow-listed visitor can explicitly request an agent-authored proposal");

    var proposals = new AssetProposalQueue(root);
    var queuedObject = await proposals.AppendAsync(home.AgentId, objectProposal!, "42", "Visitor", "10", "100", CancellationToken.None);
    Assert(queuedObject.Success, "Discord object proposal is appended");
    var inbox = Path.Combine(root, "Saved", "CaptiveSky", "ComfyBlender", "Requests", "inbox.jsonl");
    using (var queuedRecord = System.Text.Json.JsonDocument.Parse(await File.ReadAllTextAsync(inbox)))
    {
        var record = queuedRecord.RootElement;
        Assert(record.GetProperty("requester").GetString() == home.AgentId, "proposal is attributed to stable agent ID");
        Assert(record.GetProperty("pipeline").GetString() == "ComfyBlender" && record.GetProperty("status").GetString() == "pending_review", "object proposal uses compatible review schema");
        Assert(record.GetProperty("source").GetString() == "discord" && record.GetProperty("message_id").GetString() == "100", "Discord provenance is retained");
    }
    var duplicateProposal = await proposals.AppendAsync(home.AgentId, objectProposal!, "42", "Visitor", "10", "101", CancellationToken.None);
    Assert(!duplicateProposal.Success && duplicateProposal.Message.Contains("already", StringComparison.Ordinal), "pending duplicate is rejected");
    var rejectedLong = await proposals.AppendAsync(home.AgentId, objectProposal! with { Description = new string('x', 241) }, "42", "Visitor", "10", "102", CancellationToken.None);
    Assert(!rejectedLong.Success, "oversized description rejected");
    var queuedUpgrade = await proposals.AppendAsync(home.AgentId, upgradeProposal!, "42", "Visitor", "10", "103", CancellationToken.None);
    Assert(queuedUpgrade.Success, "upgrade proposal is appended");
    using (var proposalRecords = System.Text.Json.JsonDocument.Parse("[" + string.Join(',', await File.ReadAllLinesAsync(inbox)) + "]"))
        Assert(proposalRecords.RootElement[1].GetProperty("pipeline").GetString() == "human_review", "upgrade proposal remains human-review only");
    var queuedDescriptiveUpgrade = await proposals.AppendAsync(home.AgentId, descriptiveUpgrade!, "42", "Visitor", "10", "106", CancellationToken.None);
    Assert(queuedDescriptiveUpgrade.Success, "upgrade request for a descriptive landscape target is queued");
    using (var descriptiveRecords = System.Text.Json.JsonDocument.Parse("[" + string.Join(',', await File.ReadAllLinesAsync(inbox)) + "]"))
    {
        var descriptiveRecord = descriptiveRecords.RootElement.EnumerateArray()
            .Single(record => record.GetProperty("message_id").GetString() == "106");
        Assert(descriptiveRecord.GetProperty("target").GetString() == "western stone tower" &&
               descriptiveRecord.GetProperty("pipeline").GetString() == "human_review",
            "multi-word target is preserved for human review without automatic world changes");
    }
    var overRequesterLimit = await proposals.AppendAsync(home.AgentId,
        objectProposal! with { Description = "Add a sheltered survey perch" }, "42", "Visitor", "10", "107", CancellationToken.None);
    Assert(!overRequesterLimit.Success && overRequesterLimit.Message.Contains("3 proposals awaiting human review", StringComparison.Ordinal),
        "Discord cannot exceed the shared three-pending-proposal limit for one resident");
    var anotherResidentProposal = await proposals.AppendAsync("Agent_Aster_01",
        objectProposal! with { Description = "Add a small hazel seed cache" }, "42", "Visitor", "10", "108", CancellationToken.None);
    Assert(anotherResidentProposal.Success,
        "one resident reaching their pending limit does not block another resident");
    var reviewedRows = await File.ReadAllLinesAsync(inbox);
    var reviewedFirst = System.Text.Json.Nodes.JsonNode.Parse(reviewedRows[0])!.AsObject();
    reviewedFirst["status"] = "accepted";
    reviewedRows[0] = reviewedFirst.ToJsonString();
    await File.WriteAllLinesAsync(inbox, reviewedRows);
    var replacementProposal = await proposals.AppendAsync(home.AgentId,
        objectProposal! with { Description = "Add a sheltered rain gauge" }, "42", "Visitor", "10", "109", CancellationToken.None);
    Assert(replacementProposal.Success,
        "a reviewed proposal frees one of the resident's pending slots");

    var lockedRoot = Path.Combine(root, "locked-proposals");
    var lockedQueue = new AssetProposalQueue(lockedRoot);
    var lockedInboxPath = Path.Combine(lockedRoot, "Saved", "CaptiveSky", "ComfyBlender", "Requests", "inbox.jsonl");
    Directory.CreateDirectory(Path.GetDirectoryName(lockedInboxPath)!);
    var lockHolderStart = new ProcessStartInfo(Environment.ProcessPath ?? throw new InvalidOperationException("Current process path is unavailable."))
    {
        UseShellExecute = false,
        RedirectStandardOutput = true,
        RedirectStandardError = true
    };
    lockHolderStart.ArgumentList.Add(Assembly.GetEntryAssembly()?.Location
        ?? throw new InvalidOperationException("Checks assembly path is unavailable."));
    lockHolderStart.ArgumentList.Add("--hold-proposal-inbox-lock");
    lockHolderStart.ArgumentList.Add(lockedInboxPath + ".lock");
    using var lockHolder = Process.Start(lockHolderStart)
        ?? throw new InvalidOperationException("Could not start proposal lock-holder process.");
    var lockSignal = await lockHolder.StandardOutput.ReadLineAsync().WaitAsync(TimeSpan.FromSeconds(5));
    Assert(lockSignal == "LOCKED", "separate process acquires the shared inbox lock");
    var waitingAppend = lockedQueue.AppendAsync(home.AgentId,
        objectProposal! with { Description = "Add one more safely serialized proposal" }, "42", "Visitor", "10", "110", CancellationToken.None);
    await Task.Delay(100);
    var appendWaitedForOtherProcess = !waitingAppend.IsCompleted;
    await lockHolder.WaitForExitAsync().WaitAsync(TimeSpan.FromSeconds(5));
    Assert(lockHolder.ExitCode == 0, "separate process releases the shared inbox lock cleanly");
    Assert(appendWaitedForOtherProcess, "Discord waits while another process owns the shared inbox lock");
    Assert((await waitingAppend).Success, "Discord appends after the shared inbox lock is released");

    var proposalHistory = await proposals.GetRecentSummaryAsync(home.AgentId, "42", CancellationToken.None);
    Assert(proposalHistory.Contains("A copper bell", StringComparison.Ordinal) &&
           proposalHistory.Contains("Add brass trim", StringComparison.Ordinal) &&
           proposalHistory.Contains("pending human review", StringComparison.Ordinal),
        "proposal history summarizes this correspondent's own requests and review state");
    Assert(await proposals.GetRecentSummaryAsync(home.AgentId, "99", CancellationToken.None) == "(none recorded)",
        "proposal history does not expose another correspondent's requests");
    var queuedStatus = await proposals.GetStatusAsync(home.AgentId, "42", queuedUpgrade.RequestId[..8], CancellationToken.None);
    Assert(queuedStatus.Contains("awaiting human review", StringComparison.Ordinal) &&
           queuedStatus.Contains("Nothing has been generated or changed", StringComparison.Ordinal),
        "status lookup reports pending review without changing the world");
    var otherCorrespondentStatus = await proposals.GetStatusAsync(home.AgentId, "99", queuedUpgrade.RequestId[..8], CancellationToken.None);
    Assert(otherCorrespondentStatus.Contains("No proposal", StringComparison.Ordinal),
        "status lookup hides another correspondent's proposal");

    var statusFixtures = new[]
    {
        (Id: "fedcba98-0000-0000-0000-000000000001", Status: "accepted", Expected: "No world change is confirmed"),
        (Id: "badc0ffe-0000-0000-0000-000000000002", Status: "generated", Expected: "does not confirm Unreal import or placement"),
        (Id: "deadbeef-0000-0000-0000-000000000003", Status: "declined", Expected: "Nothing was generated or changed"),
    };
    foreach (var fixture in statusFixtures)
        await File.AppendAllTextAsync(inbox, System.Text.Json.JsonSerializer.Serialize(new
        {
            id = fixture.Id, requester = home.AgentId, participant_id = "42", status = fixture.Status
        }) + "\n");
    foreach (var fixture in statusFixtures)
    {
        var status = await proposals.GetStatusAsync(home.AgentId, "42", fixture.Id[..8], CancellationToken.None);
        Assert(status.Contains(fixture.Expected, StringComparison.Ordinal),
            $"status lookup accurately describes {fixture.Status} without implying world completion");
    }

    await File.AppendAllTextAsync(inbox, System.Text.Json.JsonSerializer.Serialize(new
    {
        id = "abcdef01-0000-0000-0000-000000000004", requester = home.AgentId, participant_id = "42", status = "pending_review"
    }) + "\n");
    await File.AppendAllTextAsync(inbox, System.Text.Json.JsonSerializer.Serialize(new
    {
        id = "abcdef01-1111-0000-0000-000000000005", requester = home.AgentId, participant_id = "42", status = "pending_review"
    }) + "\n");
    var ambiguousStatus = await proposals.GetStatusAsync(home.AgentId, "42", "abcdef01", CancellationToken.None);
    Assert(ambiguousStatus.Contains("more than one", StringComparison.Ordinal), "ambiguous short request IDs require more characters");
    var boundedProposalHistory = await proposals.GetRecentSummaryAsync(home.AgentId, "42", CancellationToken.None);
    Assert(boundedProposalHistory.Split('\n').Count(line => line.StartsWith("- ", StringComparison.Ordinal)) <= 5,
        "proposal history remains bounded to five recent entries");

    var brokenRoot = Path.Combine(root, "broken-queue");
    var brokenInbox = Path.Combine(brokenRoot, "Saved", "CaptiveSky", "ComfyBlender", "Requests", "inbox.jsonl");
    Directory.CreateDirectory(Path.GetDirectoryName(brokenInbox)!);
    await File.WriteAllTextAsync(brokenInbox, "{ not json }\n");
    var brokenQueue = new AssetProposalQueue(brokenRoot);
    var brokenAppend = await brokenQueue.AppendAsync(home.AgentId, objectProposal!, "42", "Visitor", "10", "104", CancellationToken.None);
    Assert(!brokenAppend.Success && brokenAppend.Message.Contains("unreadable", StringComparison.Ordinal), "unreadable inbox fails closed");
    var brokenStatus = await brokenQueue.GetStatusAsync(home.AgentId, "42", "deadbeef", CancellationToken.None);
    Assert(brokenStatus.Contains("unreadable", StringComparison.Ordinal), "status lookup fails closed on unreadable inboxes");
    var brokenHistory = await brokenQueue.GetRecentSummaryAsync(home.AgentId, "42", CancellationToken.None);
    Assert(brokenHistory.Contains("unreadable", StringComparison.Ordinal), "proposal history fails closed on unreadable inboxes");

    var fullRoot = Path.Combine(root, "full-queue");
    var fullInbox = Path.Combine(fullRoot, "Saved", "CaptiveSky", "ComfyBlender", "Requests", "inbox.jsonl");
    Directory.CreateDirectory(Path.GetDirectoryName(fullInbox)!);
    var pendingRows = Enumerable.Range(0, 64).Select(index => System.Text.Json.JsonSerializer.Serialize(new { id = $"pending-{index}", requester = $"other-{index}", description = "different request", request_type = "new_object", status = "pending_review" }));
    await File.WriteAllLinesAsync(fullInbox, pendingRows);
    var fullQueue = new AssetProposalQueue(fullRoot);
    var fullAppend = await fullQueue.AppendAsync(home.AgentId, objectProposal!, "42", "Visitor", "10", "105", CancellationToken.None);
    Assert(!fullAppend.Success && fullAppend.Message.Contains("full", StringComparison.Ordinal), "pending queue cap is enforced");

    var vanishedHome = new AgentHome(root, "Agent_Vanished_01");
    Directory.CreateDirectory(vanishedHome.DirectoryPath);
    await File.WriteAllTextAsync(Path.Combine(vanishedHome.DirectoryPath, "identity.md"), "A briefly embodied test being.");
    await File.WriteAllTextAsync(Path.Combine(vanishedHome.DirectoryPath, "personality.md"), "Resilient.");
    Directory.CreateDirectory(vanishedHome.GatewayStatePath);
    await File.WriteAllTextAsync(Path.Combine(vanishedHome.GatewayStatePath, "embodiment.json"),
        $$"""{"schema_version":1,"agent_id":"{{vanishedHome.AgentId}}","session_id":"vanishing-session","expires_at":"{{DateTimeOffset.UtcNow.AddMilliseconds(100):O}}"}""");
    var vanishedResponder = new FakeResponder(vanishedHome);
    var vanishedMemories = new AgentMemoryStore(vanishedHome);
    var vanishedRouter = new AgentMessageRouter(
        vanishedMemories,
        vanishedResponder,
        new AgentRuntimeLease(vanishedHome),
        new MessageDeduplicator(vanishedHome),
        new FileEmbodimentBridge(vanishedHome, bridgeConfiguration),
        new HeadlessTurnMarker(vanishedHome));
    var vanishedReply = await vanishedRouter.RouteAsync(
        message with { MessageId = "discord-message-3", AgentId = vanishedHome.AgentId }, CancellationToken.None);
    Assert(vanishedReply?.Speech == "Hello from the test agent.",
        "an unclaimed turn falls back headlessly after embodiment vanishes");
    Assert(vanishedResponder.CallCount == 1, "vanished embodiment does not strand an untouched turn");

    Console.WriteLine("All CaptiveSky gateway checks passed.");
    return 0;
}
catch (Exception exception)
{
    Console.Error.WriteLine($"FAIL: CaptiveSky gateway checks aborted: {exception}");
    return 1;
}
finally
{
    try
    {
        if (Directory.Exists(root))
            Directory.Delete(root, recursive: true);
    }
    catch (Exception exception) when (exception is IOException or UnauthorizedAccessException)
    {
        Console.Error.WriteLine($"WARN: Could not remove temporary gateway check data at {root}: {exception.Message}");
    }
}

static void Assert(bool condition, string description)
{
    if (!condition)
        throw new InvalidOperationException($"Check failed: {description}");
    Console.WriteLine($"PASS: {description}");
}

static void AssertThrows<TException>(Action action, string description) where TException : Exception
{
    try
    {
        action();
    }
    catch (TException)
    {
        Console.WriteLine($"PASS: {description}");
        return;
    }
    throw new InvalidOperationException($"Check failed: {description}");
}

file sealed class FakeResponder(AgentHome home, AgentProposal? proposal = null) : IAgentResponder
{
    public int CallCount { get; private set; }
    public bool SawHeadlessMarker { get; private set; }

    public Task<AgentResponse> RespondAsync(ExternalMessage message, CancellationToken cancellationToken)
    {
        CallCount++;
        SawHeadlessMarker = File.Exists(Path.Combine(home.GatewayStatePath, "headless-turn.json"));
        return Task.FromResult(new AgentResponse(
            "Hello from the test agent.",
            [new NewMemory("A visitor greeted me through an outside channel.", 0.4, ["greeting"])],
            proposal));
    }
}

file sealed class CapturingHttpMessageHandler(string responseBody) : HttpMessageHandler
{
    public string RequestBody { get; private set; } = string.Empty;

    protected override async Task<HttpResponseMessage> SendAsync(HttpRequestMessage request, CancellationToken cancellationToken)
    {
        RequestBody = await request.Content!.ReadAsStringAsync(cancellationToken);
        return new HttpResponseMessage(System.Net.HttpStatusCode.OK)
        {
            Content = new StringContent(responseBody, System.Text.Encoding.UTF8, "application/json")
        };
    }
}
