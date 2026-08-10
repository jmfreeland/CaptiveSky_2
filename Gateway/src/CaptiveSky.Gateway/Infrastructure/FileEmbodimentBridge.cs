using System.Text.Json;
using System.Text.Json.Serialization;
using CaptiveSky.Gateway.Configuration;
using CaptiveSky.Gateway.Domain;

namespace CaptiveSky.Gateway.Infrastructure;

public sealed class FileEmbodimentBridge(AgentHome home, EmbodimentConfiguration configuration)
{
    private static readonly JsonSerializerOptions JsonOptions = new() { WriteIndented = false };
    private string PresencePath => Path.Combine(home.GatewayStatePath, "embodiment.json");
    private string InboxPath => Path.Combine(home.GatewayStatePath, "inbox");
    private string OutboxPath => Path.Combine(home.GatewayStatePath, "outbox");

    public async Task<bool> IsEmbodiedAsync(CancellationToken cancellationToken)
    {
        try
        {
            if (!File.Exists(PresencePath))
                return false;
            await using var stream = new FileStream(PresencePath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
            var presence = await JsonSerializer.DeserializeAsync<EmbodimentPresence>(stream, JsonOptions, cancellationToken);
            return presence is not null &&
                   string.Equals(presence.AgentId, home.AgentId, StringComparison.Ordinal) &&
                   presence.ExpiresAt > DateTimeOffset.UtcNow;
        }
        catch (IOException)
        {
            return false;
        }
        catch (JsonException)
        {
            return false;
        }
    }

    public async Task<AgentReply?> TryRouteAsync(ExternalMessage message, CancellationToken cancellationToken)
    {
        if (!await IsEmbodiedAsync(cancellationToken))
            return null;

        var turn = ExternalTurnEnvelope.FromMessage(message);
        Directory.CreateDirectory(InboxPath);
        Directory.CreateDirectory(OutboxPath);
        var inboxFile = Path.Combine(InboxPath, $"{turn.TurnId}.json");
        await WriteAtomicallyAsync(inboxFile, turn, cancellationToken);

        var responseFile = Path.Combine(OutboxPath, $"{turn.TurnId}.json");
        var deadline = DateTimeOffset.UtcNow.AddSeconds(configuration.ResponseTimeoutSeconds);
        while (DateTimeOffset.UtcNow < deadline)
        {
            cancellationToken.ThrowIfCancellationRequested();
            if (File.Exists(responseFile))
            {
                ExternalTurnResponse? response;
                try
                {
                    await using var stream = new FileStream(responseFile, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
                    response = await JsonSerializer.DeserializeAsync<ExternalTurnResponse>(stream, JsonOptions, cancellationToken);
                }
                catch (IOException)
                {
                    await Task.Delay(configuration.PollIntervalMilliseconds, cancellationToken);
                    continue;
                }

                if (response is null || !string.Equals(response.TurnId, turn.TurnId, StringComparison.Ordinal))
                    throw new InvalidOperationException($"Embodied response for turn {turn.TurnId} was invalid.");
                File.Delete(responseFile);
                if (!string.Equals(response.Status, "completed", StringComparison.OrdinalIgnoreCase))
                    throw new InvalidOperationException($"Embodied turn failed: {response.Error}");
                return new AgentReply(response.Speech);
            }

            // If the body disappeared before claiming the inbox file, reclaim the turn safely and
            // let the router answer headlessly. A turn already moved into processing is preserved:
            // reclaiming that could create two replies while an embodied LLM request is still alive.
            if (!await IsEmbodiedAsync(cancellationToken) && File.Exists(inboxFile))
            {
                var reclaimed = inboxFile + $".{Guid.NewGuid():N}.reclaimed";
                try
                {
                    File.Move(inboxFile, reclaimed);
                    File.Delete(reclaimed);
                    return null;
                }
                catch (IOException)
                {
                    // The embodiment may have claimed it between the presence check and the move.
                }
            }
            await Task.Delay(configuration.PollIntervalMilliseconds, cancellationToken);
        }

        throw new TimeoutException(
            $"The embodied agent '{home.AgentId}' did not answer external turn {turn.TurnId} within {configuration.ResponseTimeoutSeconds} seconds. The queued turn was preserved.");
    }

    private static async Task WriteAtomicallyAsync<T>(string destination, T value, CancellationToken cancellationToken)
    {
        var temporary = destination + $".{Guid.NewGuid():N}.tmp";
        await using (var stream = new FileStream(temporary, FileMode.CreateNew, FileAccess.Write, FileShare.None))
        {
            await JsonSerializer.SerializeAsync(stream, value, JsonOptions, cancellationToken);
            await stream.FlushAsync(cancellationToken);
        }
        File.Move(temporary, destination);
    }
}

public sealed class EmbodimentPresence
{
    [JsonPropertyName("schema_version")]
    public int SchemaVersion { get; set; }

    [JsonPropertyName("agent_id")]
    public string AgentId { get; set; } = string.Empty;

    [JsonPropertyName("session_id")]
    public string SessionId { get; set; } = string.Empty;

    [JsonPropertyName("expires_at")]
    public DateTimeOffset ExpiresAt { get; set; }
}
