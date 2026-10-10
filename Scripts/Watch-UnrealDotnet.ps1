param(
    [ValidateRange(1, 360)]
    [int]$DurationMinutes = 120,

    [ValidateRange(1, 10)]
    [int]$PollSeconds = 1,

    [string]$LogPath
)

$ErrorActionPreference = 'Stop'
$StartedAt = Get-Date

if ([string]::IsNullOrWhiteSpace($LogPath)) {
    $RepoRoot = Split-Path -Parent $PSScriptRoot
    $LogDirectory = Join-Path $RepoRoot 'Saved/Diagnostics'
    New-Item -ItemType Directory -Path $LogDirectory -Force | Out-Null
    $LogPath = Join-Path $LogDirectory ('UnrealDotnetWatch_{0}.jsonl' -f $StartedAt.ToString('yyyyMMdd_HHmmss'))
} else {
    $LogDirectory = Split-Path -Parent $LogPath
    if (-not [string]::IsNullOrWhiteSpace($LogDirectory)) {
        New-Item -ItemType Directory -Path $LogDirectory -Force | Out-Null
    }
}

function Write-Record {
    param([hashtable]$Record)

    $Record.timestamp = (Get-Date).ToString('o')
    $Json = $Record | ConvertTo-Json -Compress -Depth 5
    Add-Content -LiteralPath $LogPath -Value $Json -Encoding UTF8
}

function Protect-CommandLine {
    param([string]$CommandLine)

    if ([string]::IsNullOrWhiteSpace($CommandLine)) { return $CommandLine }
    return [regex]::Replace(
        $CommandLine,
        '(?i)\b(api[_-]?key|token|secret|password)\b(\s*[=:]\s*)("[^"]*"|''[^'']*''|[^\s]+)',
        '$1$2<redacted>')
}

$TrackedProcesses = @{}
$SeenEventIds = @{}
$TargetPattern = '^(dotnet|UnrealBuildTool|UnrealEditor|UnrealEditor-Cmd|AutomationTool|CrashReportClient|CrashReportClientEditor)(\.exe)?$'
$Deadline = $StartedAt.AddMinutes($DurationMinutes)

Write-Record @{
    kind = 'watch_started'
    durationMinutes = $DurationMinutes
    pollSeconds = $PollSeconds
    logPath = $LogPath
    computer = $env:COMPUTERNAME
    powershell = $PSVersionTable.PSVersion.ToString()
}
Write-Host "Watching Unreal/.NET process activity and matching Application events for up to $DurationMinutes minutes."
Write-Host "Log: $LogPath"
Write-Host 'Press Ctrl+C to stop early.'

try {
    while ((Get-Date) -lt $Deadline) {
        $AllProcesses = @(Get-CimInstance -ClassName Win32_Process)
        $ById = @{}
        foreach ($Process in $AllProcesses) { $ById[[string]$Process.ProcessId] = $Process }

        $Current = @{}
        foreach ($Process in $AllProcesses) {
            $Name = [string]$Process.Name
            $CommandLine = [string]$Process.CommandLine
            if ($Name -notmatch $TargetPattern -and $CommandLine -notmatch 'UnrealBuildTool\.dll|AutomationTool\.dll') { continue }

            $Key = '{0}:{1}' -f $Process.ProcessId, $Process.CreationDate
            $Current[$Key] = $true
            if ($TrackedProcesses.ContainsKey($Key)) { continue }

            $ParentName = $null
            $Parent = $ById[[string]$Process.ParentProcessId]
            if ($null -ne $Parent) { $ParentName = [string]$Parent.Name }
            Write-Record @{
                kind = 'process_observed'
                processName = $Name
                processId = [uint32]$Process.ProcessId
                parentProcessId = [uint32]$Process.ParentProcessId
                parentProcessName = $ParentName
                creationDate = [string]$Process.CreationDate
                executablePath = [string]$Process.ExecutablePath
                commandLine = Protect-CommandLine $CommandLine
            }
        }

        foreach ($Key in @($TrackedProcesses.Keys)) {
            if (-not $Current.ContainsKey($Key)) {
                Write-Record @{ kind = 'process_exit_observed'; processInstance = $Key }
            }
        }
        $TrackedProcesses = $Current

        try {
            $Events = @(Get-WinEvent -FilterHashtable @{
                LogName = 'Application'
                Id = 1000, 1001, 1026
                StartTime = $StartedAt
            } -MaxEvents 40 -ErrorAction SilentlyContinue)
            foreach ($Event in $Events) {
                $EventKey = [string]$Event.RecordId
                if ($SeenEventIds.ContainsKey($EventKey)) { continue }
                $Message = [string]$Event.Message
                if ($Message -notmatch '(?i)dotnet|unreal|0xe0434352|\.NET Runtime|Common Language Runtime') { continue }

                $SeenEventIds[$EventKey] = $true
                Write-Record @{
                    kind = 'application_error_event'
                    eventId = [int]$Event.Id
                    recordId = [long]$Event.RecordId
                    provider = [string]$Event.ProviderName
                    machine = [string]$Event.MachineName
                    eventTime = $Event.TimeCreated.ToString('o')
                    message = $Message
                }
            }
        } catch {
            Write-Record @{ kind = 'event_log_read_error'; message = $_.Exception.Message }
        }

        Start-Sleep -Seconds $PollSeconds
    }
} finally {
    Write-Record @{ kind = 'watch_stopped'; logPath = $LogPath }
}
