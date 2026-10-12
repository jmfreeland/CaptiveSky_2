[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [ValidatePattern('^[0-9]{17,20}$')]
    [string]$ChannelId,

    [Parameter(Mandatory)]
    [ValidatePattern('^[0-9]{17,20}$')]
    [string]$RavenUserId,

    [Parameter(Mandatory)]
    [ValidateLength(1, 1800)]
    [string]$Message,

    [switch]$ConfirmSend
)

if (-not $ConfirmSend) {
    throw 'This sends a real Discord message as Architect. Re-run with -ConfirmSend after reviewing the exact message and destination.'
}

if ([string]::IsNullOrWhiteSpace($Message)) {
    throw 'Message must contain non-whitespace text.'
}

$token = [Environment]::GetEnvironmentVariable('CAPTIVESKY_ARCHITECT_DISCORD_TOKEN')
if ([string]::IsNullOrWhiteSpace($token)) {
    throw 'CAPTIVESKY_ARCHITECT_DISCORD_TOKEN is not set. Store the bot token as a user environment variable; never pass it on the command line.'
}

$body = [ordered]@{
    content = "<@$RavenUserId> $Message"
    allowed_mentions = @{
        parse = @()
        users = @($RavenUserId)
        replied_user = $false
    }
} | ConvertTo-Json -Depth 4 -Compress

$response = Invoke-RestMethod `
    -Uri "https://discord.com/api/v10/channels/$ChannelId/messages" `
    -Method Post `
    -Headers @{ Authorization = "Bot $token" } `
    -ContentType 'application/json' `
    -Body $body `
    -TimeoutSec 20

[pscustomobject]@{
    MessageId = $response.id
    ChannelId = $response.channel_id
    Timestamp = $response.timestamp
}
