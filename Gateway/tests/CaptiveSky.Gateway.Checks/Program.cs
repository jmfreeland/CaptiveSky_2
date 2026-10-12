using CaptiveSky.Gateway;
using CaptiveSky.Gateway.Agents;
using CaptiveSky.Gateway.Configuration;
using CaptiveSky.Gateway.Domain;
using CaptiveSky.Gateway.Infrastructure;
using CaptiveSky.Gateway.Routing;
using CaptiveSky.Gateway.Channels;
using CaptiveSky.Gateway.Proposals;

var root = Path.Combine(Path.GetTempPath(), $"CaptiveSkyGatewayChecks-{Guid.NewGuid():N}");
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
         "nests":[{"site":"Roost_West","layers":2,"builders":["Agent_Test_01"]},{"site":"Roost_East","layers":4,"builders":["Someone_Else"]}],
         "curios":[{"id":"Cairn","kind":"Cairn","state":5,"contributors":["Agent_Test_01"]}],
         "arrangement_sites":[
           {"id":"ArrangingGround_1","work":{"form":"ring","title":"Tide Watch","intent":"to mark the chime","maker":"Agent_Test_01","day":2,"responses":[]}},
           {"id":"ArrangingGround_2","work":{"form":"spiral","title":"Secret Title","intent":"secret meaning","maker":"Someone_Else","day":3,
             "responses":[{"agent":"Agent_Test_01","day":4,"intent":"an answer"}]}},
           {"id":"ArrangingGround_3"}]}
        """);
    var worldFacts = WorldStateSummary.Describe(root, home.AgentId);
    Assert(worldFacts.Contains("2 of 5 layers of a nest at Roost_West", StringComparison.Ordinal), "own nest is recalled headlessly");
    Assert(!worldFacts.Contains("Roost_East", StringComparison.Ordinal), "others' nests are not attributed to the agent");
    Assert(worldFacts.Contains("cairn; it stood 5 stones", StringComparison.Ordinal), "own cairn stones are recalled");
    Assert(worldFacts.Contains("\"Tide Watch\", made 4 Island day(s)", StringComparison.Ordinal) && worldFacts.Contains("to mark the chime", StringComparison.Ordinal), "own arrangement, age, and intent are recalled");
    Assert(worldFacts.Contains("answered someone else's stone spiral", StringComparison.Ordinal) && worldFacts.Contains("an answer", StringComparison.Ordinal), "own response is recalled");
    Assert(!worldFacts.Contains("Secret", StringComparison.Ordinal) && !worldFacts.Contains("secret", StringComparison.Ordinal) && !worldFacts.Contains("Someone_Else", StringComparison.Ordinal), "others' makers, titles, and intents stay private");
    Assert(WorldStateSummary.Describe(root, "Agent_Nobody").Length == 0, "agents who made nothing get no facts");

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

    Assert(DiscordProposalCommand.IsCommand("!request object a bell"), "explicit proposal command is recognized");
    Assert(!DiscordProposalCommand.IsCommand("!requestion object a bell"), "proposal command prefix is exact");
    Assert(DiscordProposalCommand.TryParse("!request object A copper bell", out var objectProposal, out _)
        && objectProposal is { RequestType: "new_object", Description: "A copper bell" }, "new-object proposal parses");
    Assert(DiscordProposalCommand.TryParse("!request upgrade aesthetic InnDoorLantern :: Add brass trim", out var upgradeProposal, out _)
        && upgradeProposal is { RequestType: "upgrade", UpgradeKind: "aesthetic", Target: "InnDoorLantern", Description: "Add brass trim" }, "upgrade proposal parses");
    Assert(DiscordProposalCommand.TryParse("!request upgrade variation world :: Add stream stones", out var worldUpgrade, out _)
        && worldUpgrade is { Target: "", UpgradeKind: "variation" }, "world upgrade omits a specific target");
    Assert(!DiscordProposalCommand.TryParse("!request help", out _, out var proposalHelp) && proposalHelp.Contains("human review", StringComparison.Ordinal), "help returns safe usage");
    Assert(!DiscordProposalCommand.TryParse("!request upgrade magical Inn :: Make magic", out _, out _), "unsupported upgrade kind rejected");

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

    var brokenRoot = Path.Combine(root, "broken-queue");
    var brokenInbox = Path.Combine(brokenRoot, "Saved", "CaptiveSky", "ComfyBlender", "Requests", "inbox.jsonl");
    Directory.CreateDirectory(Path.GetDirectoryName(brokenInbox)!);
    await File.WriteAllTextAsync(brokenInbox, "{ not json }\n");
    var brokenQueue = new AssetProposalQueue(brokenRoot);
    var brokenAppend = await brokenQueue.AppendAsync(home.AgentId, objectProposal!, "42", "Visitor", "10", "104", CancellationToken.None);
    Assert(!brokenAppend.Success && brokenAppend.Message.Contains("unreadable", StringComparison.Ordinal), "unreadable inbox fails closed");

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
finally
{
    if (Directory.Exists(root))
        Directory.Delete(root, recursive: true);
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

file sealed class FakeResponder(AgentHome home) : IAgentResponder
{
    public int CallCount { get; private set; }
    public bool SawHeadlessMarker { get; private set; }

    public Task<AgentResponse> RespondAsync(ExternalMessage message, CancellationToken cancellationToken)
    {
        CallCount++;
        SawHeadlessMarker = File.Exists(Path.Combine(home.GatewayStatePath, "headless-turn.json"));
        return Task.FromResult(new AgentResponse(
            "Hello from the test agent.",
            [new NewMemory("A visitor greeted me through an outside channel.", 0.4, ["greeting"])]));
    }
}
