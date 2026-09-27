using System.Text.RegularExpressions;

namespace CaptiveSky.Gateway.Agents;

/// <summary>
/// Chooses which memories accompany a headless reply. Taking only the newest records lets one
/// long in-world exchange (or a burst of near-identical reflections) crowd out everything else,
/// and the reply then just continues that exchange. Instead this keeps a balanced, chronological
/// mix: the ongoing thread with this correspondent, a few in-world conversation lines, and recent
/// distinct reflections and observations.
/// </summary>
public static partial class MemoryContextSelector
{
    public const int MaxThreadRecords = 8;
    public const int MaxInWorldConversation = 3;
    public const double NearDuplicate = 0.6;

    public static IReadOnlyList<MemoryRecord> Select(IReadOnlyList<MemoryRecord> records, string participantId, int maximumRecords)
    {
        if (maximumRecords <= 0 || records.Count == 0)
            return [];

        var participantTag = $"participant:{SanitizeTag(participantId)}";
        var thread = records.Where(record => record.Tags.Contains("external") && record.Tags.Contains(participantTag))
            .TakeLast(Math.Min(MaxThreadRecords, maximumRecords))
            .ToList();
        var chosen = new List<MemoryRecord>(thread);
        var chosenWords = chosen.Select(record => Words(record.Text)).ToList();
        var inWorldConversation = 0;

        // Walk the rest from newest to oldest, keeping only what adds something new.
        for (var index = records.Count - 1; index >= 0 && chosen.Count < maximumRecords; --index)
        {
            var record = records[index];
            if (thread.Contains(record))
                continue;
            if (record.Type == "conversation")
            {
                // Correspondence with other people is not this conversation; in-world talk is capped.
                if (record.Tags.Contains("external") || inWorldConversation >= MaxInWorldConversation)
                    continue;
            }
            var words = Words(record.Text);
            if (words.Count == 0 || chosenWords.Any(other => Similarity(words, other) >= NearDuplicate))
                continue;
            if (record.Type == "conversation")
                ++inWorldConversation;
            chosen.Add(record);
            chosenWords.Add(words);
        }

        return chosen.OrderBy(record => record.Timestamp).ToArray();
    }

    internal static double Similarity(HashSet<string> left, HashSet<string> right)
    {
        var union = left.Count + right.Count - left.Count(right.Contains);
        return union == 0 ? 0 : (double)left.Count(right.Contains) / union;
    }

    private static HashSet<string> Words(string text) =>
        NonAlphanumeric().Replace(text.ToLowerInvariant(), " ").Split(' ', StringSplitOptions.RemoveEmptyEntries).ToHashSet();

    private static string SanitizeTag(string value) =>
        new(value.ToLowerInvariant().Where(character => char.IsLetterOrDigit(character) || character is '-' or '_' or ':').ToArray());

    [GeneratedRegex("[^0-9a-z]", RegexOptions.CultureInvariant)]
    private static partial Regex NonAlphanumeric();
}
