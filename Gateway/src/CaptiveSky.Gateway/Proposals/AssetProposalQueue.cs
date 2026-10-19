using System.Text.Json;
using System.Diagnostics;
using CaptiveSky.Gateway.Domain;

namespace CaptiveSky.Gateway.Proposals;

public sealed class AssetProposalQueue(string projectRoot)
{
    private const int MaxDescriptionCharacters = 240;
    private const int MaxPending = 64;
    private const int MaxPendingPerRequester = 3;
    private const int MaxRecentHistory = 5;
    private static readonly TimeSpan InboxLockTimeout = TimeSpan.FromSeconds(5);
    private static readonly TimeSpan InboxLockRetry = TimeSpan.FromMilliseconds(25);
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
            await using var inboxLock = await AcquireInboxLockAsync(cancellationToken);
            var pending = 0;
            var requesterPending = 0;
            if (File.Exists(_path))
            {
                foreach (var line in await File.ReadAllLinesAsync(_path, cancellationToken))
                {
                    if (string.IsNullOrWhiteSpace(line)) continue;
                    using var existing = JsonDocument.Parse(line);
                    var record = existing.RootElement;
                    if (!record.TryGetProperty("status", out var status) || status.GetString() != "pending_review") continue;
                    pending++;
                    if (Get(record, "requester") == requester) requesterPending++;
                    var existingType = Get(record, "request_type");
                    if (existingType.Length == 0) existingType = "new_object";
                    if (Get(record, "requester") == requester && Get(record, "description").Trim().Equals(description, StringComparison.OrdinalIgnoreCase) &&
                        existingType == proposal.RequestType && Get(record, "target") == proposal.Target &&
                        Get(record, "upgrade_kind") == proposal.UpgradeKind)
                        return (false, "", "You already have this proposal awaiting review; nothing was added.");
                }
            }
            if (requesterPending >= MaxPendingPerRequester)
                return (false, "", $"You have {MaxPendingPerRequester} proposals awaiting human review; wait for one to be reviewed before adding another.");
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
        catch (ProposalInboxBusyException)
        {
            return (false, "", "The shared proposal inbox is busy; please try again shortly. Nothing was added.");
        }
        catch (JsonException)
        {
            return (false, "", "The proposal inbox contains an unreadable record; no request was added.");
        }
        catch (IOException)
        {
            return (false, "", "The proposal inbox could not be written; no request was added.");
        }
        catch (UnauthorizedAccessException)
        {
            return (false, "", "The proposal inbox is not writable by the gateway; no request was added.");
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
            if (!PathExists(_path))
                return "No proposal with that ID was found for this correspondent.";
            await using var inboxLock = await AcquireInboxLockAsync(cancellationToken);

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
        catch (UnauthorizedAccessException)
        {
            return "The proposal inbox is not readable by the gateway; its status is unavailable.";
        }
        finally { Gate.Release(); }
    }

    public async Task<string> GetRecentSummaryAsync(
        string requester, string participantId, CancellationToken cancellationToken)
    {
        if (string.IsNullOrWhiteSpace(requester) || string.IsNullOrWhiteSpace(participantId))
            return "(unavailable: correspondent identity is missing)";

        await Gate.WaitAsync(cancellationToken);
        try
        {
            if (!PathExists(_path)) return "(none recorded)";
            await using var inboxLock = await AcquireInboxLockAsync(cancellationToken);

            var entries = new List<string>();
            foreach (var line in await File.ReadAllLinesAsync(_path, cancellationToken))
            {
                if (string.IsNullOrWhiteSpace(line)) continue;
                using var document = JsonDocument.Parse(line);
                var record = document.RootElement;
                if (record.ValueKind != JsonValueKind.Object)
                    throw new JsonException("The proposal record must be an object.");
                if (Get(record, "requester") != requester || Get(record, "participant_id") != participantId)
                    continue;

                var id = Get(record, "id");
                if (id.Length < 8) continue;
                var requestType = Get(record, "request_type");
                var upgradeKind = Get(record, "upgrade_kind");
                var kind = requestType switch
                {
                    "upgrade" when upgradeKind is "aesthetic" or "variation" or "functionality" => $"{upgradeKind} upgrade",
                    "upgrade" => "upgrade",
                    "new_object" or "" => "new object",
                    _ => "proposal"
                };
                var status = Get(record, "status") switch
                {
                    "pending_review" => "pending human review",
                    "accepted" => "accepted for human follow-up",
                    "generated" => "candidate generated",
                    "declined" => "declined",
                    _ => "status unknown"
                };
                var target = CleanText(Get(record, "target"), 96);
                var description = CleanText(Get(record, "description"), MaxDescriptionCharacters);
                if (description.Length == 0) description = "(description unavailable)";
                entries.Add($"- {id[..8]} [{status}] {kind}{(target.Length == 0 ? "" : $" for {target}")}: {description}");
            }

            return entries.Count == 0 ? "(none recorded)" : string.Join('\n', entries.TakeLast(MaxRecentHistory));
        }
        catch (JsonException)
        {
            return "(unavailable: proposal inbox is unreadable)";
        }
        catch (IOException)
        {
            return "(unavailable: proposal inbox could not be read)";
        }
        catch (UnauthorizedAccessException)
        {
            return "(unavailable: proposal inbox is not readable by the gateway)";
        }
        finally { Gate.Release(); }
    }

    private static string Get(JsonElement record, string property) =>
        record.TryGetProperty(property, out var value) && value.ValueKind == JsonValueKind.String ? value.GetString() ?? "" : "";

    private static bool PathExists(string path)
    {
        try
        {
            File.GetAttributes(path);
            return true;
        }
        catch (FileNotFoundException) { return false; }
        catch (DirectoryNotFoundException) { return false; }
    }

    private async Task<FileStream> AcquireInboxLockAsync(CancellationToken cancellationToken)
    {
        var timer = Stopwatch.StartNew();
        var lockPath = _path + ".lock";
        while (true)
        {
            cancellationToken.ThrowIfCancellationRequested();
            try
            {
                return new FileStream(lockPath, FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.None);
            }
            catch (IOException exception) when (IsFileShareConflict(exception))
            {
                if (timer.Elapsed >= InboxLockTimeout)
                    throw new ProposalInboxBusyException("The shared proposal inbox remained locked until the timeout.", exception);
                await Task.Delay(InboxLockRetry, cancellationToken);
            }
        }
    }

    private static bool IsFileShareConflict(IOException exception) =>
        (exception.HResult & 0xFFFF) is 32 or 33; // ERROR_SHARING_VIOLATION / ERROR_LOCK_VIOLATION

    private sealed class ProposalInboxBusyException(string message, Exception innerException)
        : IOException(message, innerException);

    private static string CleanText(string value, int maximumCharacters)
    {
        var clean = string.Concat(value.Select(character => char.IsControl(character) ? ' ' : character)).Trim();
        return clean.Length <= maximumCharacters ? clean : clean[..maximumCharacters];
    }
}
