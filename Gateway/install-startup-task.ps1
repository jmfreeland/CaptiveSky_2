[CmdletBinding()]
param(
    [string]$TaskName = 'CaptiveSky Agent Gateway',
    [string]$ConfigPath = (Join-Path $PSScriptRoot 'gateway.json')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$launcher = (Resolve-Path -LiteralPath (Join-Path $PSScriptRoot 'start-gateway.ps1')).Path
$resolvedConfig = (Resolve-Path -LiteralPath $ConfigPath).Path
$quotedLauncher = '"' + $launcher.Replace('"', '`"') + '"'
$quotedConfig = '"' + $resolvedConfig.Replace('"', '`"') + '"'
$arguments = "-NoProfile -WindowStyle Hidden -ExecutionPolicy Bypass -File $quotedLauncher -ConfigPath $quotedConfig"

$action = New-ScheduledTaskAction -Execute 'powershell.exe' -Argument $arguments
$trigger = New-ScheduledTaskTrigger -AtLogOn -User $env:USERNAME
$settings = New-ScheduledTaskSettingsSet -ExecutionTimeLimit ([TimeSpan]::Zero) -RestartCount 3 -RestartInterval (New-TimeSpan -Minutes 1)
$principal = New-ScheduledTaskPrincipal -UserId $env:USERNAME -LogonType InteractiveToken -RunLevel Limited

Register-ScheduledTask -TaskName $TaskName -Action $action -Trigger $trigger -Settings $settings `
    -Principal $principal -Description 'Runs the channel-neutral CaptiveSky agent gateway while this user is logged in.' -Force | Out-Null

Write-Host "Installed scheduled task '$TaskName'. It will start at the next logon."
Write-Host "Run it now with: Start-ScheduledTask -TaskName '$TaskName'"
