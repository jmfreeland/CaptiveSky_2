using System.Text.Json;

namespace CaptiveSky.Gateway.Infrastructure;

public sealed record HeadlessTurnBudgetResult(
    bool Allowed,
    DateTimeOffset? AvailableAt = null,
    bool StateError = false);

/// <summary>Persists a rolling per-agent quota for LLM-backed headless correspondence.</summary>
public sealed class HeadlessTurnBudget
{
    public const int DefaultMaximumTurns = 64;
    public static readonly TimeSpan DefaultWindow = TimeSpan.FromMinutes(30);

    private static readonly JsonSerializerOptions JsonOptions = new() { PropertyNameCaseInsensitive = true };
    private readonly AgentHome _home;
    private readonly int _maximumTurns;
    private readonly TimeSpan _window;
    private readonly SemaphoreSlim _localLock = new(1, 1);
    private string StatePath => Path.Combine(_home.GatewayStatePath, "headless-turn-budget.json");

    public HeadlessTurnBudget(AgentHome home, int maximumTurns = DefaultMaximumTurns, TimeSpan? window = null)
    {
        if (maximumTurns <= 0)
            throw new ArgumentOutOfRangeException(nameof(maximumTurns), "The turn budget must be positive.");
        var resolvedWindow = window ?? DefaultWindow;
        if (resolvedWindow <= TimeSpan.Zero)
            throw new ArgumentOutOfRangeException(nameof(window), "The turn-budget window must be positive.");

        _home = home;
        _maximumTurns = maximumTurns;
        _window = resolvedWindow;
    }

    public async Task<HeadlessTurnBudgetResult> TryReserveAsync(DateTimeOffset now, CancellationToken cancellationToken)
    {
        await _localLock.WaitAsync(cancellationToken);
        string? temporaryPath = null;
        try
        {
            List<DateTimeOffset> reservations;
            if (File.Exists(StatePath))
            {
                try
                {
                    await using var stream = new FileStream(StatePath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
                    var state = await JsonSerializer.DeserializeAsync<BudgetState>(stream, JsonOptions, cancellationToken);
                    if (state is null || state.SchemaVersion != 1 || state.Reservations is null)
                        return new HeadlessTurnBudgetResult(false, StateError: true);
                    reservations = state.Reservations;
                }
                catch (JsonException)
                {
                    return new HeadlessTurnBudgetResult(false, StateError: true);
                }
                catch (IOException)
                {
                    return new HeadlessTurnBudgetResult(false, StateError: true);
                }
                catch (UnauthorizedAccessException)
                {
                    return new HeadlessTurnBudgetResult(false, StateError: true);
                }
            }
            else
            {
                reservations = [];
            }

            reservations.RemoveAll(timestamp => now - timestamp >= _window);
            reservations.Sort();
            if (reservations.Count >= _maximumTurns)
                return new HeadlessTurnBudgetResult(false, reservations[0] + _window);

            reservations.Add(now);
            Directory.CreateDirectory(_home.GatewayStatePath);
            temporaryPath = StatePath + $".{Guid.NewGuid():N}.tmp";
            await File.WriteAllTextAsync(temporaryPath, JsonSerializer.Serialize(new BudgetState(1, reservations)), cancellationToken);
            File.Move(temporaryPath, StatePath, overwrite: true);
            temporaryPath = null;
            return new HeadlessTurnBudgetResult(true);
        }
        catch (IOException)
        {
            return new HeadlessTurnBudgetResult(false, StateError: true);
        }
        catch (UnauthorizedAccessException)
        {
            return new HeadlessTurnBudgetResult(false, StateError: true);
        }
        finally
        {
            if (temporaryPath is not null)
            {
                try { File.Delete(temporaryPath); }
                catch (IOException) { }
                catch (UnauthorizedAccessException) { }
            }
            _localLock.Release();
        }
    }

    private sealed record BudgetState(int SchemaVersion, List<DateTimeOffset>? Reservations);
}
