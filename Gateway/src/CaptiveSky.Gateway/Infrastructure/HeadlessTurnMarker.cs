using System.Text.Json;

namespace CaptiveSky.Gateway.Infrastructure;

public sealed class HeadlessTurnMarker(AgentHome home)
{
    public async Task<IAsyncDisposable> MarkActiveAsync(TimeSpan maximumDuration, CancellationToken cancellationToken)
    {
        Directory.CreateDirectory(home.GatewayStatePath);
        var path = Path.Combine(home.GatewayStatePath, "headless-turn.json");
        var temporary = path + $".{Guid.NewGuid():N}.tmp";
        var json = JsonSerializer.Serialize(new
        {
            schema_version = 1,
            agent_id = home.AgentId,
            holder = $"gateway:{Environment.MachineName}:{Environment.ProcessId}",
            started_at = DateTimeOffset.UtcNow,
            expires_at = DateTimeOffset.UtcNow.Add(maximumDuration)
        });
        await File.WriteAllTextAsync(temporary, json, cancellationToken);
        File.Move(temporary, path, overwrite: true);
        return new Releaser(path);
    }

    private sealed class Releaser(string path) : IAsyncDisposable
    {
        public ValueTask DisposeAsync()
        {
            try
            {
                File.Delete(path);
            }
            catch (IOException)
            {
            }
            return ValueTask.CompletedTask;
        }
    }
}
