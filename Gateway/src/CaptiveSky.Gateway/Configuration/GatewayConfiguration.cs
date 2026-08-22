using System.Text.Json;
using System.Text.Json.Serialization;

namespace CaptiveSky.Gateway.Configuration;

public sealed class GatewayConfiguration
{
    public string ProjectRoot { get; set; } = "../..";
    public LlmConfiguration Llm { get; set; } = new();
    public EmbodimentConfiguration Embodiment { get; set; } = new();
    public List<AgentConfiguration> Agents { get; set; } = [];

    [JsonIgnore]
    public string ResolvedProjectRoot { get; private set; } = string.Empty;

    public static GatewayConfiguration Load(string path)
    {
        var fullPath = Path.GetFullPath(path);
        var json = File.ReadAllText(fullPath);
        var options = new JsonSerializerOptions
        {
            PropertyNameCaseInsensitive = true,
            ReadCommentHandling = JsonCommentHandling.Skip,
            AllowTrailingCommas = true
        };
        var configuration = JsonSerializer.Deserialize<GatewayConfiguration>(json, options)
            ?? throw new InvalidOperationException($"Gateway configuration '{fullPath}' was empty.");

        var configurationDirectory = Path.GetDirectoryName(fullPath)!;
        configuration.ResolvedProjectRoot = Path.GetFullPath(configuration.ProjectRoot, configurationDirectory);
        configuration.Validate();
        return configuration;
    }

    public void Validate()
    {
        if (!Directory.Exists(ResolvedProjectRoot))
            throw new DirectoryNotFoundException($"Project root does not exist: {ResolvedProjectRoot}");
        if (string.IsNullOrWhiteSpace(Llm.Endpoint))
            throw new InvalidOperationException("llm.endpoint is required.");
        if (string.IsNullOrWhiteSpace(Llm.Model))
            throw new InvalidOperationException("llm.model is required.");

        var duplicate = Agents.GroupBy(agent => agent.AgentId, StringComparer.OrdinalIgnoreCase)
            .FirstOrDefault(group => group.Count() > 1);
        if (duplicate is not null)
            throw new InvalidOperationException($"Agent '{duplicate.Key}' is configured more than once.");

        foreach (var agent in Agents)
        {
            AgentHome.ValidateAgentId(agent.AgentId);
            if (agent.Discord.Enabled && string.IsNullOrWhiteSpace(agent.Discord.TokenEnvironmentVariable))
                throw new InvalidOperationException($"Agent '{agent.AgentId}' has Discord enabled without a token environment variable.");
        }
    }
}

public sealed class EmbodimentConfiguration
{
    public int ResponseTimeoutSeconds { get; set; } = 120;
    public int PollIntervalMilliseconds { get; set; } = 250;
}

public sealed class LlmConfiguration
{
    public string Endpoint { get; set; } = "https://api.openai.com/v1/chat/completions";
    public string Model { get; set; } = "gpt-5.6-luna";
    public string ApiKeyEnvironmentVariable { get; set; } = "OPENAI_API_KEY";
    public bool RequireApiKey { get; set; } = true;
    public int MaxCompletionTokens { get; set; } = 900;
    public double Temperature { get; set; } = 1.0;
    public int MemoryContextRecords { get; set; } = 20;
}

public sealed class AgentConfiguration
{
    public string AgentId { get; set; } = string.Empty;
    public bool Enabled { get; set; } = true;
    public DiscordConfiguration Discord { get; set; } = new();
}

public sealed class DiscordConfiguration
{
    public bool Enabled { get; set; }
    public string TokenEnvironmentVariable { get; set; } = string.Empty;
    public bool AllowDirectMessages { get; set; } = true;
    public bool RespondToMentions { get; set; } = true;
    public List<ulong> AllowedUserIds { get; set; } = [];
    public List<ulong> AllowedGuildIds { get; set; } = [];
    public List<ulong> AllowedChannelIds { get; set; } = [];
}
