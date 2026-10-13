using System.Text.Json;
using CaptiveSky.Gateway.Domain;

namespace CaptiveSky.Gateway.Proposals;

public sealed class AssetProposalQueue(string projectRoot)
{
    private const int MaxDescriptionCharacters = 240;
    private const int MaxPending = 64;
    private static readonly SemaphoreSlim Gate = new(1, 1);
    private readonly string _path = Path.Combine(Path.GetFullPath(projectRoot), "Saved", "CaptiveSky", "ComfyBlender", "Requests", "inbox.jsonl");

    public async Task<(bool Success, string RequestId, string Message)> AppendAsync(
        string requester, AgentProposal proposal, string participantId, string participantName,
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

    public async Task<string> GetStatusAsync(
        string requester, string participantId, string requestIdPrefix, CancellationToken cancellationToken)
    {
        if (string.IsNullOrWhiteSpace(requester) || string.IsNullOrWhiteSpace(participantId) ||
            requestIdPrefix.Length is < 8 or > 36 ||
            requestIdPrefix.Any(character => !char.IsAsciiHexDigit(character) && character != '-'))
            return "Use `!request status <8-character-id>` to check one of your proposals.";

        await Gate.WaitAsync(cancellationToken);
        try
        {
            if (!File.Exists(_path))
                return "No proposal with that ID was found for this correspondent.";

            var matches = new List<(string Id, string Status)>();
            foreach (var line in await File.ReadAllLinesAsync(_path, cancellationToken))
            {
                if (string.IsNullOrWhiteSpace(line)) continue;
                using var document = JsonDocument.Parse(line);
                var record = document.RootElement;
                if (record.ValueKind != JsonValueKind.Object)
                    throw new JsonException("The proposal record must be an object.");
                var id = Get(record, "id");
                if (Get(record, "requester") != requester || Get(record, "participant_id") != participantId ||
                    !id.StartsWith(requestIdPrefix, StringComparison.OrdinalIgnoreCase))
                    continue;
                matches.Add((id, Get(record, "status")));
            }

            if (matches.Count == 0)
                return "No proposal with that ID was found for this correspondent.";
            if (matches.Count > 1)
                return "That short ID matches more than one proposal; include more characters.";

            var match = matches[0];
            var explanation = match.Status switch
            {
                "pending_review" => "is awaiting human review. Nothing has been generated or changed in the world.",
                "accepted" => "was accepted for human follow-up. No world change is confirmed yet.",
                "generated" => "has a generated asset candidate. This does not confirm Unreal import or placement.",
                "declined" => "was declined. Nothing was generated or changed in the world.",
                _ => "has an unrecognized status. Please ask the human reviewer for details."
            };
            return $"Proposal `{match.Id[..8]}` {explanation}";
        }
        catch (JsonException)
        {
            return "The proposal inbox contains an unreadable record; its status is unavailable.";
        }
        catch (IOException)
        {
            return "The proposal inbox could not be read; its status is unavailable.";
        }
        finally { Gate.Release(); }
    }

    private static string Get(JsonElement record, string property) =>
        record.TryGetProperty(property, out var value) && value.ValueKind == JsonValueKind.String ? value.GetString() ?? "" : "";
}
