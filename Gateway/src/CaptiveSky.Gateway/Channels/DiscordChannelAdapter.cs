using CaptiveSky.Gateway.Configuration;
using CaptiveSky.Gateway.Domain;
using Discord;
using Discord.WebSocket;

namespace CaptiveSky.Gateway.Channels;

public sealed class DiscordChannelAdapter : IChannelAdapter
{
    private readonly AgentConfiguration _agent;
    private readonly DiscordSocketClient _client;
    private ExternalMessageHandler? _handler;
    private CancellationToken _shutdownToken;

    public DiscordChannelAdapter(AgentConfiguration agent)
    {
        _agent = agent;
        _client = new DiscordSocketClient(new DiscordSocketConfig
        {
            GatewayIntents = GatewayIntents.Guilds |
                             GatewayIntents.GuildMessages |
                             GatewayIntents.DirectMessages |
                             GatewayIntents.MessageContent,
            MessageCacheSize = 20
        });
        _client.Log += message =>
        {
            Console.WriteLine($"[{DateTimeOffset.Now:T}] Discord/{_agent.AgentId} {message.Severity}: {message.Message}");
            return Task.CompletedTask;
        };
        _client.MessageReceived += message =>
        {
            _ = Task.Run(() => HandleMessageAsync(message), _shutdownToken);
            return Task.CompletedTask;
        };
    }

    public async Task StartAsync(ExternalMessageHandler handler, CancellationToken cancellationToken)
    {
        _handler = handler;
        _shutdownToken = cancellationToken;
        var token = Environment.GetEnvironmentVariable(_agent.Discord.TokenEnvironmentVariable);
        if (string.IsNullOrWhiteSpace(token))
            throw new InvalidOperationException(
                $"Discord token environment variable '{_agent.Discord.TokenEnvironmentVariable}' is not set for {_agent.AgentId}.");

        var ready = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        Task OnReady()
        {
            ready.TrySetResult();
            return Task.CompletedTask;
        }
        _client.Ready += OnReady;
        try
        {
            await _client.LoginAsync(TokenType.Bot, token);
            await _client.StartAsync();
            await ready.Task.WaitAsync(TimeSpan.FromSeconds(30), cancellationToken);
            Console.WriteLine($"Discord adapter ready: {_agent.AgentId} as {_client.CurrentUser.Username}.");
        }
        finally
        {
            _client.Ready -= OnReady;
        }
    }

    public async Task StopAsync(CancellationToken cancellationToken)
    {
        await _client.StopAsync();
        await _client.LogoutAsync();
    }

    public async ValueTask DisposeAsync()
    {
        await _client.DisposeAsync();
    }

    private async Task HandleMessageAsync(SocketMessage socketMessage)
    {
        try
        {
            if (_handler is null || socketMessage is not SocketUserMessage message || message.Author.IsBot)
                return;
            if (!IsAllowedUser(message.Author.Id) || !IsAddressedToAgent(message))
                return;

            var text = RemoveBotMention(message.Content).Trim();
            if (string.IsNullOrWhiteSpace(text))
                return;

            using var typing = message.Channel.EnterTypingState();
            var guildChannel = message.Channel as SocketGuildChannel;
            var externalMessage = new ExternalMessage(
                message.Id.ToString(),
                _agent.AgentId,
                "discord",
                guildChannel is null
                    ? $"discord:dm:{message.Channel.Id}"
                    : $"discord:guild:{guildChannel.Guild.Id}:channel:{message.Channel.Id}",
                new ExternalParticipant(message.Author.Id.ToString(), message.Author.GlobalName ?? message.Author.Username),
                text,
                message.Timestamp,
                new Dictionary<string, string>
                {
                    ["channel_id"] = message.Channel.Id.ToString(),
                    ["guild_id"] = guildChannel?.Guild.Id.ToString() ?? string.Empty
                });
            var reply = await _handler(externalMessage, _shutdownToken);
            if (reply is null)
                return;

            foreach (var part in SplitMessage(reply.Speech, 1900))
                await message.Channel.SendMessageAsync(part, messageReference: new MessageReference(message.Id));
        }
        catch (OperationCanceledException) when (_shutdownToken.IsCancellationRequested)
        {
        }
        catch (Exception exception)
        {
            Console.Error.WriteLine($"Discord message failed for {_agent.AgentId}: {exception}");
        }
    }

    private bool IsAllowedUser(ulong userId) =>
        _agent.Discord.AllowedUserIds.Count == 0 || _agent.Discord.AllowedUserIds.Contains(userId);

    private bool IsAddressedToAgent(SocketUserMessage message)
    {
        if (message.Channel is SocketDMChannel)
            return _agent.Discord.AllowDirectMessages;

        if (message.Channel is not SocketGuildChannel guildChannel)
            return false;
        if (_agent.Discord.AllowedGuildIds.Count > 0 && !_agent.Discord.AllowedGuildIds.Contains(guildChannel.Guild.Id))
            return false;
        if (_agent.Discord.AllowedChannelIds.Count > 0 && !_agent.Discord.AllowedChannelIds.Contains(message.Channel.Id))
            return false;
        if (!_agent.Discord.RespondToMentions)
            return false;

        if (message.MentionedUsers.Any(user => user.Id == _client.CurrentUser.Id))
            return true;

        // Pasted mentions render like @Raven but contain no Discord mention entity.
        // Keep this opt-in per agent and retain the user/guild/channel allow-lists above.
        var aliases = _agent.Discord.TextualMentionAliases.Count > 0
            ? _agent.Discord.TextualMentionAliases
            : [_client.CurrentUser.Username];
        return aliases.Any(alias => HasTextualMention(message.Content, alias));
    }

    private string RemoveBotMention(string content)
    {
        var cleaned = content
        .Replace($"<@{_client.CurrentUser.Id}>", string.Empty, StringComparison.Ordinal)
        .Replace($"<@!{_client.CurrentUser.Id}>", string.Empty, StringComparison.Ordinal);
        var aliases = _agent.Discord.TextualMentionAliases.Count > 0
            ? _agent.Discord.TextualMentionAliases
            : [_client.CurrentUser.Username];
        foreach (var alias in aliases)
        {
            if (HasTextualMention(cleaned, alias))
            {
                var trimmed = cleaned.TrimStart();
                cleaned = trimmed[(alias.TrimStart('@').Length + 1)..];
                break;
            }
        }
        return cleaned;
    }

    public static bool HasTextualMention(string content, string alias)
    {
        if (string.IsNullOrWhiteSpace(alias)) return false;
        var trimmed = content.TrimStart();
        var prefix = "@" + alias.TrimStart('@');
        if (!trimmed.StartsWith(prefix, StringComparison.OrdinalIgnoreCase)) return false;
        return trimmed.Length == prefix.Length || char.IsWhiteSpace(trimmed[prefix.Length]) || char.IsPunctuation(trimmed[prefix.Length]);
    }

    internal static IEnumerable<string> SplitMessage(string message, int maximumLength)
    {
        for (var offset = 0; offset < message.Length; offset += maximumLength)
            yield return message.Substring(offset, Math.Min(maximumLength, message.Length - offset));
    }
}
