using CaptiveSky.Gateway.Domain;

namespace CaptiveSky.Gateway.Proposals;

public static class DiscordProposalCommand
{
    public const string Usage = "Examples: `!request object A small copper bell beside the east trail`; `!request upgrade aesthetic InnDoorLantern :: Add brass trim`; `!request upgrade variation world :: Add mossy stream stones`; `!request upgrade functionality RavenNest :: Add a sheltered landing rail`. You can also ask Raven to submit one specific idea for human review. These are proposals only; nothing is created automatically.";

    public static bool IsCommand(string text) =>
        text.StartsWith("!request", StringComparison.OrdinalIgnoreCase) &&
        (text.Length == 8 || char.IsWhiteSpace(text[8]));

    public static bool TryParse(string text, out AgentProposal? proposal, out string error)
    {
        proposal = null;
        error = Usage;
        if (!IsCommand(text)) return false;
        var args = text[8..].Trim();
        if (args.Equals("help", StringComparison.OrdinalIgnoreCase) || args.Length == 0) return false;

        if (args.StartsWith("object ", StringComparison.OrdinalIgnoreCase))
        {
            proposal = new AgentProposal("new_object", args[7..].Trim());
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
        proposal = new AgentProposal("upgrade", fields[1], target, kind);
        error = string.Empty;
        return true;
    }
}
