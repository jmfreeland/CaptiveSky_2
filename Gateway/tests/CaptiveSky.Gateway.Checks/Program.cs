using CaptiveSky.Gateway;
using CaptiveSky.Gateway.Agents;
using CaptiveSky.Gateway.Configuration;
using CaptiveSky.Gateway.Domain;
using CaptiveSky.Gateway.Infrastructure;
using CaptiveSky.Gateway.Routing;

var root = Path.Combine(Path.GetTempPath(), $"CaptiveSkyGatewayChecks-{Guid.NewGuid():N}");
try
{
    var home = new AgentHome(root, "Agent_Test_01");
    Directory.CreateDirectory(home.DirectoryPath);
    await File.WriteAllTextAsync(Path.Combine(home.DirectoryPath, "identity.md"), "A test being.");
    await File.WriteAllTextAsync(Path.Combine(home.DirectoryPath, "personality.md"), "Careful and concise.");

    Assert(home.LoadRequiredDocument("identity.md") == "A test being.", "agent documents load");
    AssertThrows<ArgumentException>(() => new AgentHome(root, "../escape"), "agent ids cannot escape the project");

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
