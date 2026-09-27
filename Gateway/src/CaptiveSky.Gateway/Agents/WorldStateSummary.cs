using System.Text;
using System.Text.Json;

namespace CaptiveSky.Gateway.Agents;

/// <summary>
/// Summarizes the lasting world changes an agent itself made, from Unreal's per-level
/// <c>WorldState/&lt;Map&gt;.json</c> files, so headless correspondence can remember them.
/// Only the agent's own contributions are included, mirroring what its body would know:
/// who made other things, and other residents' private titles or intents, are never revealed.
/// A missing or unreadable file simply contributes nothing.
/// </summary>
public static class WorldStateSummary
{
    private const int MaxFacts = 8;

    public static string Describe(string projectRoot, string agentId)
    {
        var directory = Path.Combine(projectRoot, "WorldState");
        if (!Directory.Exists(directory))
            return string.Empty;

        var facts = new List<string>();
        foreach (var path in Directory.EnumerateFiles(directory, "*.json").Order(StringComparer.Ordinal))
        {
            try
            {
                using var json = JsonDocument.Parse(File.ReadAllText(path));
                DescribeLevel(Path.GetFileNameWithoutExtension(path), json.RootElement, agentId, facts);
            }
            catch (Exception exception) when (exception is IOException or JsonException or UnauthorizedAccessException or InvalidOperationException)
            {
                // Unreal owns these files; a partial or locked read just means no facts this turn.
            }
        }
        if (facts.Count == 0)
            return string.Empty;

        var builder = new StringBuilder();
        foreach (var fact in facts.Take(MaxFacts))
            builder.Append("- ").AppendLine(fact);
        return builder.ToString().TrimEnd();
    }

    private static void DescribeLevel(string level, JsonElement root, string agentId, List<string> facts)
    {
        var day = root.TryGetProperty("clock", out var clock) && clock.TryGetProperty("day", out var dayNode) && dayNode.TryGetInt32(out var savedDay)
            ? savedDay
            : (int?)null;

        foreach (var nest in Items(root, "nests"))
        {
            if (!Contains(nest, "builders", agentId))
                continue;
            facts.Add($"On {level}, you have woven {Int(nest, "layers")} of 5 layers of a nest at {Text(nest, "site")}.");
        }

        foreach (var curio in Items(root, "curios"))
        {
            if (Text(curio, "kind") == "Cairn" && Contains(curio, "contributors", agentId))
                facts.Add($"On {level}, you have added stones to the small cairn; it stood {Int(curio, "state")} stones high when last recorded.");
        }

        foreach (var site in Items(root, "arrangement_sites"))
        {
            if (!site.TryGetProperty("work", out var work) || work.ValueKind != JsonValueKind.Object)
                continue;
            var form = Text(work, "form");
            var age = day is { } today && work.TryGetProperty("day", out var madeNode) && madeNode.TryGetInt32(out var made)
                ? $", made {Math.Max(0, today - made)} Island day(s) before the Island was last visited"
                : string.Empty;
            if (Text(work, "maker") == agentId)
            {
                var intent = Text(work, "intent");
                facts.Add($"On {level}, you arranged stones into a {form} you call \"{Text(work, "title")}\"{age}" +
                          (string.IsNullOrEmpty(intent) ? "." : $"; you meant it as: {intent}."));
                continue;
            }
            foreach (var response in Items(work, "responses"))
            {
                if (Text(response, "agent") != agentId)
                    continue;
                var intent = Text(response, "intent");
                facts.Add($"On {level}, you answered someone else's stone {form} with a small arc of stones" +
                          (string.IsNullOrEmpty(intent) ? "." : $"; you meant: {intent}."));
            }
        }
    }

    private static IEnumerable<JsonElement> Items(JsonElement element, string name) =>
        element.TryGetProperty(name, out var array) && array.ValueKind == JsonValueKind.Array
            ? array.EnumerateArray().Where(item => item.ValueKind == JsonValueKind.Object)
            : [];

    private static string Text(JsonElement element, string name) =>
        element.TryGetProperty(name, out var node) && node.ValueKind == JsonValueKind.String ? node.GetString()?.Trim() ?? string.Empty : string.Empty;

    private static int Int(JsonElement element, string name) =>
        element.TryGetProperty(name, out var node) && node.TryGetInt32(out var value) ? value : 0;

    private static bool Contains(JsonElement element, string name, string value) =>
        element.TryGetProperty(name, out var array) && array.ValueKind == JsonValueKind.Array &&
        array.EnumerateArray().Any(item => item.ValueKind == JsonValueKind.String && item.GetString() == value);
}
