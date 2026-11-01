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
    HeadlessTurnMarker headlessTurnMarker,
    HeadlessTurnBudget headlessTurnBudget)
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
        var reservation = await headlessTurnBudget.TryReserveAsync(DateTimeOffset.UtcNow, cancellationToken);
        if (!reservation.Allowed)
        {
            await deduplicator.MarkProcessedAsync(message.Source, message.MessageId, cancellationToken);
            if (reservation.AvailableAt is DateTimeOffset availableAt)
            {
                var minutes = Math.Max(1, (int)Math.Ceiling((availableAt - DateTimeOffset.UtcNow).TotalMinutes));
                var unit = minutes == 1 ? "minute" : "minutes";
                return new AgentReply($"I've reached my headless reply limit. I haven't read or saved this message; please send it again in about {minutes} {unit}.");
            }
            return new AgentReply("I can't verify my headless reply allowance right now, so I haven't read or saved this message. Please try again later.");
        }

        await memories.AppendConversationAsync(message, message.Text, spokenByAgent: false, cancellationToken);
        var response = await responder.RespondAsync(message, cancellationToken);
        foreach (var memory in response.NewMemories)
            await memories.AppendReflectionAsync(memory.Text, memory.Importance, memory.Tags, cancellationToken);
        await memories.AppendConversationAsync(message, response.Speech, spokenByAgent: true, cancellationToken);
        await deduplicator.MarkProcessedAsync(message.Source, message.MessageId, cancellationToken);
        return new AgentReply(response.Speech, response.Proposal);
    }
}
