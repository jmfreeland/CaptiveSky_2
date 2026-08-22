using CaptiveSky.Gateway;
using CaptiveSky.Gateway.Agents;
using CaptiveSky.Gateway.Channels;
using CaptiveSky.Gateway.Configuration;
using CaptiveSky.Gateway.Infrastructure;
using CaptiveSky.Gateway.Routing;

var arguments = args.ToList();
var checkOnly = arguments.Remove("--check");
var readinessOnly = arguments.Remove("--ready");
var configArgument = arguments.FirstOrDefault(argument => !argument.StartsWith("--", StringComparison.Ordinal));
var configPath = configArgument ?? Path.Combine(AppContext.BaseDirectory, "gateway.json");
if (!File.Exists(configPath))
{
    var repositoryDefault = Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, "..", "..", "..", "..", "..", "gateway.json"));
    if (File.Exists(repositoryDefault))
        configPath = repositoryDefault;
}

try
{
    var configuration = GatewayConfiguration.Load(configPath);
    Console.WriteLine($"CaptiveSky gateway using project: {configuration.ResolvedProjectRoot}");

    var activeAgents = configuration.Agents.Where(agent => agent.Enabled && agent.Discord.Enabled).ToArray();
    var missingSecrets = new List<string>();
    foreach (var agent in activeAgents)
    {
        var home = new AgentHome(configuration.ResolvedProjectRoot, agent.AgentId);
        home.LoadRequiredDocument("identity.md");
        home.LoadRequiredDocument("personality.md");
        var tokenPresent = !string.IsNullOrWhiteSpace(Environment.GetEnvironmentVariable(agent.Discord.TokenEnvironmentVariable));
        Console.WriteLine($"  {agent.AgentId}: agent home OK; Discord token {(tokenPresent ? "present" : "missing")} ({agent.Discord.TokenEnvironmentVariable})");
        if (!tokenPresent)
            missingSecrets.Add(agent.Discord.TokenEnvironmentVariable);
    }

    var llmKeyPresent = !string.IsNullOrWhiteSpace(Environment.GetEnvironmentVariable(configuration.Llm.ApiKeyEnvironmentVariable));
    Console.WriteLine($"  Headless LLM key {(llmKeyPresent ? "present" : "missing")} ({configuration.Llm.ApiKeyEnvironmentVariable})");
    if (configuration.Llm.RequireApiKey && !llmKeyPresent)
        missingSecrets.Add(configuration.Llm.ApiKeyEnvironmentVariable);

    if (checkOnly)
        return 0;
    if (readinessOnly)
    {
        if (activeAgents.Length == 0)
            throw new InvalidOperationException("No enabled agent Discord routes were found.");
        if (missingSecrets.Count > 0)
        {
            Console.Error.WriteLine($"Gateway is not ready: missing {string.Join(", ", missingSecrets.Distinct())}.");
            return 2;
        }
        Console.WriteLine("Gateway readiness check passed.");
        return 0;
    }
    if (activeAgents.Length == 0)
        throw new InvalidOperationException("No enabled agent Discord routes were found.");

    using var shutdown = new CancellationTokenSource();
    Console.CancelKeyPress += (_, eventArgs) =>
    {
        eventArgs.Cancel = true;
        shutdown.Cancel();
    };

    using var httpClient = new HttpClient { Timeout = TimeSpan.FromSeconds(60) };
    var adapters = new List<IChannelAdapter>();
    try
    {
        foreach (var agent in activeAgents)
        {
            var home = new AgentHome(configuration.ResolvedProjectRoot, agent.AgentId);
            var memories = new AgentMemoryStore(home);
            var responder = new OpenAICompatibleAgentResponder(httpClient, configuration.Llm, home, memories);
            var router = new AgentMessageRouter(
                memories,
                responder,
                new AgentRuntimeLease(home),
                new MessageDeduplicator(home),
                new FileEmbodimentBridge(home, configuration.Embodiment),
                new HeadlessTurnMarker(home));
            var adapter = new DiscordChannelAdapter(agent);
            await adapter.StartAsync(router.RouteAsync, shutdown.Token);
            adapters.Add(adapter);
        }

        Console.WriteLine("Gateway is listening. Press Ctrl+C to stop.");
        await Task.Delay(Timeout.InfiniteTimeSpan, shutdown.Token);
    }
    catch (OperationCanceledException) when (shutdown.IsCancellationRequested)
    {
    }
    finally
    {
        foreach (var adapter in adapters)
        {
            await adapter.StopAsync(CancellationToken.None);
            await adapter.DisposeAsync();
        }
    }
    return 0;
}
catch (Exception exception)
{
    Console.Error.WriteLine($"Gateway failed: {exception.Message}");
    return 1;
}
