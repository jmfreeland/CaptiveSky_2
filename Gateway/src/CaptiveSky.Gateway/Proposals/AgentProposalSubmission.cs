using CaptiveSky.Gateway.Configuration;
using CaptiveSky.Gateway.Domain;

namespace CaptiveSky.Gateway.Proposals;

public static class AgentProposalSubmission
{
    public static async Task<string> SubmitAsync(
        AssetProposalQueue queue,
        string agentId,
        DiscordConfiguration discord,
        AgentProposal proposal,
        ulong authorId,
        string participantName,
        string channelId,
        string messageId,
        CancellationToken cancellationToken)
    {
        if (!discord.AllowAgentProposals)
            return "I can discuss a request, but agent-submitted proposals are disabled for this Discord route.";
        if (discord.AllowedUserIds.Count == 0 || !discord.AllowedUserIds.Contains(authorId))
            return "I can only submit a proposal when this route has an explicit allowedUserIds list and your Discord user ID is on it.";

        var queued = await queue.AppendAsync(
            agentId,
            proposal,
            authorId.ToString(),
            participantName,
            channelId,
            messageId,
            cancellationToken);
        return queued.Message;
    }
}
