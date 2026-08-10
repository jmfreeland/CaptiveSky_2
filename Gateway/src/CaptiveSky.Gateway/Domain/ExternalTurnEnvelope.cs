using System.Text.Json.Serialization;

namespace CaptiveSky.Gateway.Domain;

public sealed class ExternalTurnEnvelope
{
    [JsonPropertyName("schema_version")]
    public int SchemaVersion { get; set; } = 1;

    [JsonPropertyName("turn_id")]
    public string TurnId { get; set; } = Guid.NewGuid().ToString("N");

    [JsonPropertyName("agent_id")]
    public string AgentId { get; set; } = string.Empty;

    [JsonPropertyName("source")]
    public string Source { get; set; } = string.Empty;

    [JsonPropertyName("conversation_id")]
    public string ConversationId { get; set; } = string.Empty;

    [JsonPropertyName("message_id")]
    public string MessageId { get; set; } = string.Empty;

    [JsonPropertyName("sender")]
    public ExternalTurnSender Sender { get; set; } = new();

    [JsonPropertyName("text")]
    public string Text { get; set; } = string.Empty;

    [JsonPropertyName("received_at")]
    public DateTimeOffset ReceivedAt { get; set; }

    [JsonPropertyName("metadata")]
    public IReadOnlyDictionary<string, string> Metadata { get; set; } = new Dictionary<string, string>();

    public static ExternalTurnEnvelope FromMessage(ExternalMessage message) => new()
    {
        AgentId = message.AgentId,
        Source = message.Source,
        ConversationId = message.ConversationId,
        MessageId = message.MessageId,
        Sender = new ExternalTurnSender { Id = message.Sender.Id, DisplayName = message.Sender.DisplayName },
        Text = message.Text,
        ReceivedAt = message.Timestamp,
        Metadata = message.Metadata ?? new Dictionary<string, string>()
    };
}

public sealed class ExternalTurnSender
{
    [JsonPropertyName("id")]
    public string Id { get; set; } = string.Empty;

    [JsonPropertyName("display_name")]
    public string DisplayName { get; set; } = string.Empty;
}

public sealed class ExternalTurnResponse
{
    [JsonPropertyName("schema_version")]
    public int SchemaVersion { get; set; } = 1;

    [JsonPropertyName("turn_id")]
    public string TurnId { get; set; } = string.Empty;

    [JsonPropertyName("agent_id")]
    public string AgentId { get; set; } = string.Empty;

    [JsonPropertyName("status")]
    public string Status { get; set; } = string.Empty;

    [JsonPropertyName("speech")]
    public string Speech { get; set; } = string.Empty;

    [JsonPropertyName("error")]
    public string Error { get; set; } = string.Empty;

    [JsonPropertyName("completed_at")]
    public DateTimeOffset CompletedAt { get; set; }
}
