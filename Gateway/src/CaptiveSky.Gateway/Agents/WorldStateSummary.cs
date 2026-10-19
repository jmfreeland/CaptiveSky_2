using System.Text;
using System.Text.Json;
using System.Globalization;

namespace CaptiveSky.Gateway.Agents;

/// <summary>
/// Summarizes the latest saved environment and lasting world changes from Unreal's per-level
/// <c>WorldState/&lt;Map&gt;.json</c> files, so headless correspondence can answer from grounded context.
/// Environment observations are public and timestamped; only the agent's own lasting contributions
/// are described, so other residents' private titles or intents are never revealed.
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

        if (root.TryGetProperty("environment", out var environment) && environment.ValueKind == JsonValueKind.Object &&
            environment.TryGetProperty("observed_utc", out var observedNode) && observedNode.ValueKind == JsonValueKind.String &&
            DateTimeOffset.TryParse(observedNode.GetString(), CultureInfo.InvariantCulture,
                DateTimeStyles.AssumeUniversal | DateTimeStyles.AdjustToUniversal, out var observedUtc))
        {
            var conditions = new List<string>();
            if (Number(environment, "rain_intensity", out var rain))
                conditions.Add(rain >= 0.55 ? "steady rain" : rain > 0.02 ? "light rain" : "no rain recorded");
            if (Number(environment, "wetness", out var wetness))
                conditions.Add($"ground wetness {Percent(wetness)}");
            if (Number(environment, "cloud_cover", out var clouds))
                conditions.Add($"cloud cover {Percent(clouds)}");
            if (Number(environment, "mist", out var mist) && mist >= 0.05)
                conditions.Add($"mist {Percent(mist)}");
            if (Number(environment, "wind_speed_cm_s", out var windSpeed) &&
                Number(environment, "wind_x_cm_s", out var windX) && Number(environment, "wind_y_cm_s", out var windY))
            {
                var direction = windSpeed < 1
                    ? "calm"
                    : FormattableString.Invariant($"moving toward map X {windX / windSpeed:+0.00;-0.00;0.00}, Y {windY / windSpeed:+0.00;-0.00;0.00} (not a compass bearing)");
                conditions.Add(FormattableString.Invariant($"wind {windSpeed:0} cm/s, {direction}"));
            }
            if (conditions.Count > 0)
                facts.Insert(0, $"On {level}, the last saved local weather snapshot (sampled near the active viewpoint when present, otherwise near the weather actor) was observed at {observedUtc.ToUniversalTime():O}: {string.Join(", ", conditions)}. This is historical, not live weather.");
        }

        foreach (var nest in Items(root, "nests"))
        {
            if (!Contains(nest, "builders", agentId))
                continue;
            facts.Add($"On {level}, you have woven {Int(nest, "layers")} of 5 layers of a nest at {Text(nest, "site")}.");
        }

        foreach (var home in Items(root, "wildlife_homes"))
        {
            if (Text(home, "occupant") != agentId)
                continue;

            var species = Text(home, "species");
            if (string.IsNullOrEmpty(species))
                continue;

            var id = Text(home, "id");
            var homeName = string.IsNullOrEmpty(id) ? string.Empty : $" \"{id}\"";
            var state = Text(home, "shelter_state") switch
            {
                "Exposed" => "exposed",
                "Temporary" => "temporary",
                "Settled" => "settled",
                "Disturbed" => "disturbed",
                _ => "recorded with an unknown condition",
            };
            var quality = home.TryGetProperty("shelter_quality", out var qualityNode) && qualityNode.TryGetInt32(out var score)
                ? $" (shelter quality {Math.Clamp(score, 0, 5)}/5)"
                : string.Empty;
            var supplies = Items(home, "cached_supplies")
                .Select(cache => (Item: Text(cache, "item"), Quantity: Int(cache, "quantity")))
                .Where(cache => IsSafeIdentifier(cache.Item) && cache.Quantity is > 0 and <= 20)
                .Take(4)
                .Select(cache => $"{cache.Quantity} × {cache.Item}")
                .ToArray();
            var cacheSummary = supplies.Length == 0 ? string.Empty : $" Your last saved cache there lists {string.Join(", ", supplies)}.";
            facts.Add($"On {level}, your {species} home{homeName} was last saved as {state}{quality}.{cacheSummary}");
        }

        foreach (var curio in Items(root, "curios"))
        {
            if (Text(curio, "kind") == "Cairn" && Contains(curio, "contributors", agentId))
                facts.Add($"On {level}, you have added stones to the small cairn; it stood {Int(curio, "state")} stones high when last recorded.");
        }

        foreach (var tradition in Items(root, "craft_traditions"))
        {
            var id = Text(tradition, "id");
            var name = Text(tradition, "display_name");
            if (!IsSafeIdentifier(id) || !IsSafeCraftName(name))
                continue;

            var beganTradition = Text(tradition, "origin") == agentId;
            var learnedTradition = Items(tradition, "lessons").Any(lesson => Text(lesson, "learner") == agentId);
            var ownAttempts = Items(tradition, "attempts")
                .Where(attempt => Text(attempt, "practitioner") == agentId)
                .Select(DescribeCraftAttempt)
                .Where(summary => summary is not null)
                .TakeLast(2)
                .ToArray();

            if (!beganTradition && !learnedTradition && ownAttempts.Length == 0)
                continue;

            var details = new List<string>();
            if (beganTradition)
                details.Add($"you began the craft tradition \"{name}\"");
            else if (learnedTradition)
                details.Add($"you learned the craft tradition \"{name}\"");
            if (ownAttempts.Length > 0)
                details.Add($"your latest recorded practice: {string.Join("; ", ownAttempts)}");
            facts.Add($"On {level}, {string.Join("; ", details)}. These are resident notes, not verified recipes or crafted items.");
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

    private static bool IsSafeIdentifier(string value) =>
        value.Length is > 0 and <= 48 && value.All(character => char.IsAsciiLetterOrDigit(character) || character is '_' or '-');

    private static bool IsSafeCraftName(string value) =>
        value.Length is > 0 and <= 80 && value.All(character => char.IsAsciiLetterOrDigit(character) || character is ' ' or '_' or '-' or '\'');

    private static string? DescribeCraftAttempt(JsonElement attempt)
    {
        if (!attempt.TryGetProperty("day", out var dayNode) || !dayNode.TryGetInt32(out var day) || day is < 1 or > 10_000_000)
            return null;

        var outcome = Text(attempt, "outcome") switch
        {
            "Unrated" => "unrated experiment",
            "Promising" => "promising",
            "Successful" => "successful",
            "NeedsRevision" => "needs revision",
            _ => null,
        };
        if (outcome is null)
            return null;

        var parameters = Items(attempt, "parameters")
            .Select(parameter => (Name: Text(parameter, "id"), Value: Number(parameter, "value", out var value) ? value : double.NaN))
            .Where(parameter => (parameter.Name is "hydration" or "rise" or "heat" or "salt") &&
                                double.IsFinite(parameter.Value) && parameter.Value is >= 0 and <= 1)
            .Select(parameter => $"{parameter.Name} {Percent(parameter.Value)}")
            .ToArray();
        var settings = parameters.Length == 0 ? string.Empty : $"; intended settings: {string.Join(", ", parameters)}";
        return FormattableString.Invariant($"Island day {day}: {outcome}{settings}");
    }

    private static int Int(JsonElement element, string name) =>
        element.TryGetProperty(name, out var node) && node.TryGetInt32(out var value) ? value : 0;

    private static bool Number(JsonElement element, string name, out double value)
    {
        value = 0;
        if (!element.TryGetProperty(name, out var node) || !node.TryGetDouble(out var parsed) || !double.IsFinite(parsed))
            return false;
        value = parsed;
        return true;
    }

    private static string Percent(double value) => FormattableString.Invariant($"{Math.Round(Math.Clamp(value, 0, 1) * 100):0}%");

    private static bool Contains(JsonElement element, string name, string value) =>
        element.TryGetProperty(name, out var array) && array.ValueKind == JsonValueKind.Array &&
        array.EnumerateArray().Any(item => item.ValueKind == JsonValueKind.String && item.GetString() == value);
}
