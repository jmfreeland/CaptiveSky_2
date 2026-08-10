namespace CaptiveSky.Gateway.Domain;

public sealed record ExternalParticipant(string Id, string DisplayName);

public sealed record ExternalMessage(
    string MessageId,
    string AgentId,
    string Source,
    string ConversationId,
    ExternalParticipant Sender,
    string Text,
    DateTimeOffset Timestamp,
    IReadOnlyDictionary<string, string>? Metadata = null);

public sealed record AgentReply(string Speech);
