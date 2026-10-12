using System.Text.Json;

namespace CaptiveSky.Gateway.Proposals;

public sealed class AssetProposalQueue(string projectRoot)
{
    private const int MaxDescriptionCharacters = 240;
    private const int MaxPending = 64;
    private static readonly SemaphoreSlim Gate = new(1, 1);
    private readonly string _path = Path.Combine(Path.GetFullPath(projectRoot), "Saved", "CaptiveSky", "ComfyBlender", "Requests", "inbox.jsonl");

    public async Task<(bool Success, string RequestId, string Message)> AppendAsync(
        string requester, DiscordProposal proposal, string participantId, string participantName,
        string channelId, string messageId, CancellationToken cancellationToken)
    {
        var description = proposal.Description.Trim();
        if (string.IsNullOrWhiteSpace(requester) || description.Length is 0 or > MaxDescriptionCharacters || description.Any(char.IsControl))
            return (false, "", $"A proposal needs a single-line description of 1 to {MaxDescriptionCharacters} characters.");
        if (proposal.Target.Length > 96 || proposal.Target.Any(char.IsControl))
            return (false, "", "The target must be a short, single-line identifier.");
        if (proposal.RequestType is not ("new_object" or "upgrade") ||
            (proposal.RequestType == "upgrade" && proposal.UpgradeKind is not ("aesthetic" or "variation" or "functionality")))
            return (false, "", "That proposal type is not supported.");

        await Gate.WaitAsync(cancellationToken);
        try
        {
            Directory.CreateDirectory(Path.GetDirectoryName(_path)!);
            var pending = 0;
            if (File.Exists(_path))
            {
                foreach (var line in await File.ReadAllLinesAsync(_path, cancellationToken))
                {
                    if (string.IsNullOrWhiteSpace(line)) continue;
                    using var existing = JsonDocument.Parse(line);
                    var record = existing.RootElement;
                    if (!record.TryGetProperty("status", out var status) || status.GetString() != "pending_review") continue;
                    pending++;
                    var existingType = Get(record, "request_type");
                    if (existingType.Length == 0) existingType = "new_object";
                    if (Get(record, "requester") == requester && Get(record, "description").Trim().Equals(description, StringComparison.OrdinalIgnoreCase) &&
                        existingType == proposal.RequestType && Get(record, "target") == proposal.Target &&
                        Get(record, "upgrade_kind") == proposal.UpgradeKind)
                        return (false, "", "You already have this proposal awaiting review; nothing was added.");
                }
            }
            if (pending >= MaxPending) return (false, "", "The human-review proposal inbox is full; no request was added.");

            var id = Guid.NewGuid().ToString("D");
            var entry = new Dictionary<string, object?>
            {
                ["id"] = id, ["created_at_utc"] = DateTimeOffset.UtcNow, ["requester"] = requester,
                ["description"] = description, ["request_type"] = proposal.RequestType,
                ["pipeline"] = proposal.RequestType == "new_object" ? "ComfyBlender" : "human_review",
                ["status"] = "pending_review", ["source"] = "discord", ["participant_id"] = participantId,
                ["participant_name"] = participantName, ["channel_id"] = channelId, ["message_id"] = messageId
            };
            if (proposal.Target.Length > 0) entry["target"] = proposal.Target;
            if (proposal.UpgradeKind.Length > 0) entry["upgrade_kind"] = proposal.UpgradeKind;
            await File.AppendAllTextAsync(_path, JsonSerializer.Serialize(entry) + "\n", cancellationToken);
            return (true, id, $"Proposal `{id[..8]}` is queued for human review. Nothing has been generated or changed in the world.");
        }
        catch (JsonException)
        {
            return (false, "", "The proposal inbox contains an unreadable record; no request was added.");
        }
        catch (IOException)
        {
            return (false, "", "The proposal inbox could not be written; no request was added.");
        }
        finally { Gate.Release(); }
    }

    private static string Get(JsonElement record, string property) =>
        record.TryGetProperty(property, out var value) && value.ValueKind == JsonValueKind.String ? value.GetString() ?? "" : "";
}
