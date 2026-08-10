namespace CaptiveSky.Gateway.Infrastructure;

public sealed class MessageDeduplicator(AgentHome home)
{
    private readonly SemaphoreSlim _lock = new(1, 1);
    private HashSet<string>? _processed;
    private string StatePath => Path.Combine(home.GatewayStatePath, "processed-message-ids.txt");

    public async Task<bool> WasProcessedAsync(string source, string messageId, CancellationToken cancellationToken)
    {
        await _lock.WaitAsync(cancellationToken);
        try
        {
            await EnsureLoadedAsync(cancellationToken);
            return _processed!.Contains(Key(source, messageId));
        }
        finally
        {
            _lock.Release();
        }
    }

    public async Task MarkProcessedAsync(string source, string messageId, CancellationToken cancellationToken)
    {
        await _lock.WaitAsync(cancellationToken);
        try
        {
            await EnsureLoadedAsync(cancellationToken);
            var key = Key(source, messageId);
            if (!_processed!.Add(key))
                return;
            Directory.CreateDirectory(home.GatewayStatePath);
            await File.AppendAllTextAsync(StatePath, key + Environment.NewLine, cancellationToken);
        }
        finally
        {
            _lock.Release();
        }
    }

    private async Task EnsureLoadedAsync(CancellationToken cancellationToken)
    {
        if (_processed is not null)
            return;
        _processed = File.Exists(StatePath)
            ? new HashSet<string>(await File.ReadAllLinesAsync(StatePath, cancellationToken), StringComparer.Ordinal)
            : new HashSet<string>(StringComparer.Ordinal);
    }

    private static string Key(string source, string messageId) => $"{source.ToLowerInvariant()}:{messageId}";
}
