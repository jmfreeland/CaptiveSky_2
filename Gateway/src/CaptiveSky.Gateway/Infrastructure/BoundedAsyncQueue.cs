using System.Threading.Channels;

namespace CaptiveSky.Gateway.Infrastructure;

/// <summary>A bounded FIFO for inbound work so event sources cannot create an unbounded task backlog.</summary>
public sealed class BoundedAsyncQueue<T>
{
    private readonly Channel<T> _channel;

    public BoundedAsyncQueue(int capacity)
    {
        if (capacity <= 0)
            throw new ArgumentOutOfRangeException(nameof(capacity), "Queue capacity must be positive.");

        _channel = Channel.CreateBounded<T>(new BoundedChannelOptions(capacity)
        {
            FullMode = BoundedChannelFullMode.Wait,
            SingleReader = true,
            SingleWriter = false
        });
    }

    public bool TryEnqueue(T item) => _channel.Writer.TryWrite(item);

    public IAsyncEnumerable<T> ReadAllAsync(CancellationToken cancellationToken = default) =>
        _channel.Reader.ReadAllAsync(cancellationToken);

    public void Complete() => _channel.Writer.TryComplete();
}
