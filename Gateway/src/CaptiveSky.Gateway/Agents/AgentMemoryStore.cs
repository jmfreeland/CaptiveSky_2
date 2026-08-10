using System.Text.Json;
using System.Text.Json.Serialization;
using CaptiveSky.Gateway.Domain;

namespace CaptiveSky.Gateway.Agents;

public sealed class AgentMemoryStore(AgentHome home)
{
    private readonly SemaphoreSlim _writeLock = new(1, 1);
    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull
    };

    public async Task AppendConversationAsync(
        ExternalMessage message,
        string text,
        bool spokenByAgent,
        CancellationToken cancellationToken)
    {
        var tags = new[]
        {
            "conversation",
            "external",
            message.Source.ToLowerInvariant(),
            spokenByAgent ? "speech" : "visitor",
            $"participant:{SanitizeTag(message.Sender.Id)}"
        };
        var prefix = spokenByAgent
            ? $"I replied to {message.Sender.DisplayName} via {message.Source}:"
            : $"{message.Sender.DisplayName} wrote to me via {message.Source}:";

        await AppendAsync(new MemoryRecord
        {
            Id = Guid.NewGuid().ToString("D"),
            Timestamp = DateTimeOffset.UtcNow,
            Type = "conversation",
            Text = $"{prefix} \"{text}\"",
            Importance = 0.5,
            Tags = tags,
            Location = new MemoryLocation()
        }, cancellationToken);
    }

    public Task AppendReflectionAsync(string text, double importance, IEnumerable<string>? tags, CancellationToken cancellationToken)
    {
        return AppendAsync(new MemoryRecord
        {
            Id = Guid.NewGuid().ToString("D"),
            Timestamp = DateTimeOffset.UtcNow,
            Type = "reflection",
            Text = text,
            Importance = Math.Clamp(importance, 0, 1),
            Tags = (tags ?? []).Select(SanitizeTag).Where(tag => tag.Length > 0).ToArray(),
            Location = new MemoryLocation()
        }, cancellationToken);
    }

    public async Task<IReadOnlyList<MemoryRecord>> ReadRecentAsync(int maximumRecords, CancellationToken cancellationToken)
    {
        if (!File.Exists(home.MemoryPath) || maximumRecords <= 0)
            return [];

        var records = new List<MemoryRecord>();
        await using var stream = new FileStream(home.MemoryPath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
        using var reader = new StreamReader(stream);
        while (await reader.ReadLineAsync(cancellationToken) is { } line)
        {
            try
            {
                var record = JsonSerializer.Deserialize<MemoryRecord>(line, JsonOptions);
                if (record is not null)
                    records.Add(record);
            }
            catch (JsonException)
            {
                // Preserve append-only recovery: one malformed historical line must not hide later memories.
            }
        }

        return records.TakeLast(maximumRecords).ToArray();
    }

    private async Task AppendAsync(MemoryRecord record, CancellationToken cancellationToken)
    {
        Directory.CreateDirectory(home.DirectoryPath);
        var line = JsonSerializer.Serialize(record, JsonOptions) + Environment.NewLine;
        await _writeLock.WaitAsync(cancellationToken);
        try
        {
            await using var stream = new FileStream(home.MemoryPath, FileMode.Append, FileAccess.Write, FileShare.Read);
            await using var writer = new StreamWriter(stream);
            await writer.WriteAsync(line.AsMemory(), cancellationToken);
        }
        finally
        {
            _writeLock.Release();
        }
    }

    private static string SanitizeTag(string value) =>
        new(value.ToLowerInvariant().Where(character => char.IsLetterOrDigit(character) || character is '-' or '_' or ':').ToArray());
}

public sealed class MemoryRecord
{
    [JsonPropertyName("id")]
    public string Id { get; set; } = string.Empty;

    [JsonPropertyName("timestamp")]
    public DateTimeOffset Timestamp { get; set; }

    [JsonPropertyName("type")]
    public string Type { get; set; } = "observation";

    [JsonPropertyName("text")]
    public string Text { get; set; } = string.Empty;

    [JsonPropertyName("tags")]
    public IReadOnlyList<string> Tags { get; set; } = [];

    [JsonPropertyName("importance")]
    public double Importance { get; set; } = 0.5;

    [JsonPropertyName("location")]
    public MemoryLocation Location { get; set; } = new();
}

public sealed class MemoryLocation
{
    [JsonPropertyName("x")]
    public double X { get; set; }

    [JsonPropertyName("y")]
    public double Y { get; set; }

    [JsonPropertyName("z")]
    public double Z { get; set; }
}
