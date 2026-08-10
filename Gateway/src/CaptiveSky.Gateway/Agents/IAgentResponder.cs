using CaptiveSky.Gateway.Domain;

namespace CaptiveSky.Gateway.Agents;

public interface IAgentResponder
{
    Task<AgentResponse> RespondAsync(ExternalMessage message, CancellationToken cancellationToken);
}

public sealed record AgentResponse(string Speech, IReadOnlyList<NewMemory> NewMemories);

public sealed record NewMemory(string Text, double Importance, IReadOnlyList<string> Tags);
