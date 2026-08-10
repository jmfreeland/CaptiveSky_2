using CaptiveSky.Gateway.Agents;
using CaptiveSky.Gateway.Domain;
using CaptiveSky.Gateway.Infrastructure;

namespace CaptiveSky.Gateway.Routing;

public sealed class AgentMessageRouter(
    AgentMemoryStore memories,
    IAgentResponder responder,
    AgentRuntimeLease runtimeLease,
    MessageDeduplicator deduplicator,
    FileEmbodimentBridge embodimentBridge,
    HeadlessTurnMarker headlessTurnMarker)
{
    public async Task<AgentReply?> RouteAsync(ExternalMessage message, CancellationToken cancellationToken)
    {
        if (string.IsNullOrWhiteSpace(message.Text))
            return null;
        if (await deduplicator.WasProcessedAsync(message.Source, message.MessageId, cancellationToken))
            return null;

        await using var lease = await runtimeLease.AcquireAsync(TimeSpan.FromSeconds(30), cancellationToken);
        if (await deduplicator.WasProcessedAsync(message.Source, message.MessageId, cancellationToken))
            return null;

        var embodiedReply = await embodimentBridge.TryRouteAsync(message, cancellationToken);
        if (embodiedReply is not null)
        {
            await deduplicator.MarkProcessedAsync(message.Source, message.MessageId, cancellationToken);
            return embodiedReply;
        }

        var headlessMarkerLease = await headlessTurnMarker.MarkActiveAsync(TimeSpan.FromMinutes(2), cancellationToken);
        if (await embodimentBridge.IsEmbodiedAsync(cancellationToken))
        {
            // Close the startup race: a body may have appeared after the first presence check but
            // before the headless marker was published. Yield authority and route through it.
            await headlessMarkerLease.DisposeAsync();
            var lateEmbodiedReply = await embodimentBridge.TryRouteAsync(message, cancellationToken);
            if (lateEmbodiedReply is not null)
            {
                await deduplicator.MarkProcessedAsync(message.Source, message.MessageId, cancellationToken);
                return lateEmbodiedReply;
            }
            headlessMarkerLease = await headlessTurnMarker.MarkActiveAsync(TimeSpan.FromMinutes(2), cancellationToken);
        }

        await using var headlessMarker = headlessMarkerLease;
        await memories.AppendConversationAsync(message, message.Text, spokenByAgent: false, cancellationToken);
        var response = await responder.RespondAsync(message, cancellationToken);
        foreach (var memory in response.NewMemories)
            await memories.AppendReflectionAsync(memory.Text, memory.Importance, memory.Tags, cancellationToken);
        await memories.AppendConversationAsync(message, response.Speech, spokenByAgent: true, cancellationToken);
        await deduplicator.MarkProcessedAsync(message.Source, message.MessageId, cancellationToken);
        return new AgentReply(response.Speech);
    }
}
