[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [ValidatePattern('^[0-9]{17,20}$')]
    [string]$ChannelId,

    [ValidateRange(1, 100)]
    [int]$Limit = 50
)

$token = [Environment]::GetEnvironmentVariable('CAPTIVESKY_ARCHITECT_DISCORD_TOKEN')
if ([string]::IsNullOrWhiteSpace($token)) {
    throw 'CAPTIVESKY_ARCHITECT_DISCORD_TOKEN is not set. Store the bot token as a user environment variable; never pass it on the command line.'
}

$messages = Invoke-RestMethod `
    -Uri "https://discord.com/api/v10/channels/$ChannelId/messages?limit=$Limit" `
    -Method Get `
    -Headers @{ Authorization = "Bot $token" } `
    -TimeoutSec 20

# Discord returns newest first; reverse it so a conversation reads naturally.
@($messages) |
    Sort-Object { [DateTimeOffset]::Parse($_.timestamp) } |
    ForEach-Object {
        $authorName = $_.author.global_name
        if ([string]::IsNullOrWhiteSpace($authorName)) {
            $authorName = $_.author.username
        }
        [pscustomobject]@{
            Timestamp       = $_.timestamp
            Author          = $authorName
            IsBot           = [bool]$_.author.bot
            Content         = $_.content
            AttachmentCount = @($_.attachments).Count
            MessageId       = $_.id
        }
    }
