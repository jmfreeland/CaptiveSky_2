[CmdletBinding()]
param(
    [string]$ConfigPath = (Join-Path $PSScriptRoot 'gateway.json'),
    [ValidateRange(1, 300)]
    [int]$RestartDelaySeconds = 10,
    [switch]$SingleRun
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$gatewayProject = Join-Path $PSScriptRoot 'src\CaptiveSky.Gateway\CaptiveSky.Gateway.csproj'
$resolvedConfig = (Resolve-Path -LiteralPath $ConfigPath).Path
$logDirectory = Join-Path $PSScriptRoot 'logs'
New-Item -ItemType Directory -Path $logDirectory -Force | Out-Null
$logPath = Join-Path $logDirectory ('gateway-{0:yyyy-MM-dd}.log' -f (Get-Date))

Start-Transcript -Path $logPath -Append | Out-Null
try {
    & dotnet run --project $gatewayProject -- --ready $resolvedConfig
    if ($LASTEXITCODE -ne 0) {
        throw "Gateway readiness check failed with exit code $LASTEXITCODE."
    }

    do {
        Write-Host "[$(Get-Date -Format o)] Starting CaptiveSky gateway."
        & dotnet run --project $gatewayProject -- $resolvedConfig
        $gatewayExitCode = $LASTEXITCODE
        Write-Warning "Gateway exited with code $gatewayExitCode."
        if ($SingleRun) {
            exit $gatewayExitCode
        }
        Start-Sleep -Seconds $RestartDelaySeconds
    } while ($true)
}
finally {
    Stop-Transcript | Out-Null
}
