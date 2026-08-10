using System.Text.RegularExpressions;

namespace CaptiveSky.Gateway;

public sealed partial class AgentHome
{
    public AgentHome(string projectRoot, string agentId)
    {
        ValidateAgentId(agentId);
        AgentId = agentId;
        DirectoryPath = Path.GetFullPath(Path.Combine(projectRoot, "Agents", agentId));
    }

    public string AgentId { get; }
    public string DirectoryPath { get; }
    public string MemoryPath => Path.Combine(DirectoryPath, "memory.jsonl");
    public string GatewayStatePath => Path.Combine(DirectoryPath, ".gateway");

    public string LoadRequiredDocument(string fileName)
    {
        if (fileName is not ("identity.md" or "personality.md"))
            throw new ArgumentException("Only authored agent documents may be loaded.", nameof(fileName));

        var path = Path.Combine(DirectoryPath, fileName);
        if (!File.Exists(path))
            throw new FileNotFoundException($"Agent '{AgentId}' is missing {fileName}.", path);
        return File.ReadAllText(path).Trim();
    }

    public static void ValidateAgentId(string agentId)
    {
        if (string.IsNullOrWhiteSpace(agentId) || !SafeAgentId().IsMatch(agentId))
            throw new ArgumentException($"Invalid agent id '{agentId}'. Use letters, numbers, underscores, and hyphens only.");
    }

    [GeneratedRegex("^[A-Za-z0-9_-]+$", RegexOptions.CultureInvariant)]
    private static partial Regex SafeAgentId();
}
