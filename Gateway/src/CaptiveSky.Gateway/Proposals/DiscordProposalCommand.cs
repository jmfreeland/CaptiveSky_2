namespace CaptiveSky.Gateway.Proposals;

public sealed record DiscordProposal(string RequestType, string Description, string Target = "", string UpgradeKind = "");

public static class DiscordProposalCommand
{
    public const string Usage = "Use `!request object <description>` or `!request upgrade <aesthetic|variation|functionality> <target|world> :: <description>`. Requests go to human review; nothing is created automatically.";

    public static bool IsCommand(string text) =>
        text.StartsWith("!request", StringComparison.OrdinalIgnoreCase) &&
        (text.Length == 8 || char.IsWhiteSpace(text[8]));

    public static bool TryParse(string text, out DiscordProposal? proposal, out string error)
    {
        proposal = null;
        error = Usage;
        if (!IsCommand(text)) return false;
        var args = text[8..].Trim();
        if (args.Equals("help", StringComparison.OrdinalIgnoreCase) || args.Length == 0) return false;

        if (args.StartsWith("object ", StringComparison.OrdinalIgnoreCase))
        {
            proposal = new DiscordProposal("new_object", args[7..].Trim());
            error = string.Empty;
            return true;
        }

        if (!args.StartsWith("upgrade ", StringComparison.OrdinalIgnoreCase)) return false;
        var fields = args[8..].Split("::", 2, StringSplitOptions.TrimEntries);
        if (fields.Length != 2) return false;
        var header = fields[0].Split(' ', 2, StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
        if (header.Length != 2) return false;
        var kind = header[0].ToLowerInvariant();
        if (kind is not ("aesthetic" or "variation" or "functionality")) return false;
        var target = header[1].Equals("world", StringComparison.OrdinalIgnoreCase) ? "" : header[1];
        proposal = new DiscordProposal("upgrade", fields[1], target, kind);
        error = string.Empty;
        return true;
    }
}
