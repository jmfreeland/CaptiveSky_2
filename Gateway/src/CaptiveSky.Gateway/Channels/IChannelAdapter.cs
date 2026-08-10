using CaptiveSky.Gateway.Domain;

namespace CaptiveSky.Gateway.Channels;

public delegate Task<AgentReply?> ExternalMessageHandler(ExternalMessage message, CancellationToken cancellationToken);

public interface IChannelAdapter : IAsyncDisposable
{
    Task StartAsync(ExternalMessageHandler handler, CancellationToken cancellationToken);
    Task StopAsync(CancellationToken cancellationToken);
}
