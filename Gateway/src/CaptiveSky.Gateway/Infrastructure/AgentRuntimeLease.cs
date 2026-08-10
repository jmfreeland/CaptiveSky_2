using System.Text;
using System.Text.Json;

namespace CaptiveSky.Gateway.Infrastructure;

/// <summary>
/// Serializes headless thought for one agent across gateway processes. The lock handle is held for
/// the full turn; a future Unreal handover adapter can use the same lease boundary.
/// </summary>
public sealed class AgentRuntimeLease(AgentHome home)
{
    private readonly SemaphoreSlim _localLock = new(1, 1);

    public async Task<IAsyncDisposable> AcquireAsync(TimeSpan timeout, CancellationToken cancellationToken)
    {
        await _localLock.WaitAsync(cancellationToken);
        Directory.CreateDirectory(home.GatewayStatePath);
        var lockPath = Path.Combine(home.GatewayStatePath, "runtime.lock");
        var deadline = DateTimeOffset.UtcNow + timeout;
        try
        {
            while (true)
            {
                cancellationToken.ThrowIfCancellationRequested();
                try
                {
                    var stream = new FileStream(lockPath, FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.None);
                    stream.SetLength(0);
                    var state = JsonSerializer.Serialize(new
                    {
                        holder = $"gateway:{Environment.MachineName}:{Environment.ProcessId}",
                        acquired_at = DateTimeOffset.UtcNow
                    });
                    var bytes = Encoding.UTF8.GetBytes(state);
                    await stream.WriteAsync(bytes, cancellationToken);
                    await stream.FlushAsync(cancellationToken);
                    return new Releaser(stream, _localLock);
                }
                catch (IOException) when (DateTimeOffset.UtcNow < deadline)
                {
                    await Task.Delay(100, cancellationToken);
                }
            }
        }
        catch
        {
            _localLock.Release();
            throw;
        }
    }

    private sealed class Releaser(FileStream stream, SemaphoreSlim localLock) : IAsyncDisposable
    {
        public ValueTask DisposeAsync()
        {
            stream.Dispose();
            localLock.Release();
            return ValueTask.CompletedTask;
        }
    }
}
