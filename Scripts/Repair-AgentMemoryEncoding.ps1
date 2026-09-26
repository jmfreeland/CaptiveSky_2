#requires -Version 7.2
[CmdletBinding()]
param(
    [string[]]$AgentIds = @('Agent_Aster_01', 'Agent_Raven_01'),
    [switch]$Repair,
    [switch]$SelfTest
)
$ErrorActionPreference = 'Stop'

function Convert-MixedMemoryBytes([byte[]]$Bytes) {
    $utf8 = [System.Text.UTF8Encoding]::new($false, $true)
    $utf16 = [System.Text.UnicodeEncoding]::new($false, $false, $true)
    $utf16be = [System.Text.UnicodeEncoding]::new($true, $false, $true)
    $position = 0
    $lines = [System.Collections.Generic.List[string]]::new()
    while ($position -lt $Bytes.Length) {
        $wide = $false
        $big = $false
        if ($position + 1 -lt $Bytes.Length -and $Bytes[$position] -eq 255 -and $Bytes[$position + 1] -eq 254) {
            $wide = $true; $position += 2
        } elseif ($position + 1 -lt $Bytes.Length -and $Bytes[$position] -eq 254 -and $Bytes[$position + 1] -eq 255) {
            $wide = $true; $big = $true; $position += 2
        } elseif ($position + 2 -lt $Bytes.Length -and $Bytes[$position] -eq 239 -and $Bytes[$position + 1] -eq 187 -and $Bytes[$position + 2] -eq 191) {
            $position += 3
        }
        $start = $position
        if ($wide) {
            while ($position + 1 -lt $Bytes.Length) {
                $newline = if ($big) { $Bytes[$position] -eq 0 -and $Bytes[$position + 1] -eq 10 } else { $Bytes[$position] -eq 10 -and $Bytes[$position + 1] -eq 0 }
                $position += 2
                if ($newline) { break }
            }
            if (($position - $start) % 2 -ne 0 -or $position -eq $start) { throw 'Truncated UTF-16 record; original left unchanged.' }
            $decoder = if ($big) { $utf16be } else { $utf16 }
            $line = $decoder.GetString($Bytes, $start, $position - $start).Trim()
        } else {
            while ($position -lt $Bytes.Length -and $Bytes[$position] -ne 10) { $position++ }
            if ($position -lt $Bytes.Length) { $position++ }
            $line = $utf8.GetString($Bytes, $start, $position - $start).Trim()
        }
        if ($line.Length -eq 0) { continue }
        $record = $line | ConvertFrom-Json -ErrorAction Stop
        if (-not $record.id -or -not $record.timestamp -or -not $record.type -or $null -eq $record.text) { throw 'Invalid memory record; original left unchanged.' }
        # Preserve the original JSON and every field; no paraphrasing or deduplication.
        $lines.Add($line)
    }
    return ,$lines.ToArray()
}

if ($SelfTest) {
    $u8 = [Text.UTF8Encoding]::new($false, $true)
    $u16 = [Text.UnicodeEncoding]::new($false, $true, $true)
    $a = '{"id":"a","timestamp":"2026-09-14T00:00:00Z","type":"observation","text":"Plain ASCII"}'
    $b = '{"id":"b","timestamp":"2026-09-14T00:00:01Z","type":"conversation","text":"I am here — café 🪶"}'
    $b = $b.Replace('I am', ('I' + [char]0x2019 + 'm'))
    $c = '{"id":"c","timestamp":"2026-09-14T00:00:02Z","type":"observation","text":"風 and é"}'
    [byte[]]$mixed = $u8.GetBytes($a + "`r`n") + $u16.GetPreamble() + $u16.GetBytes($b + "`r`n") + $u8.GetBytes($c + "`n")
    $result = Convert-MixedMemoryBytes $mixed
    if ($result.Count -ne 3 -or $result[0] -cne $a -or $result[1] -cne $b -or $result[2] -cne $c) { throw 'Mixed encoding regression failed.' }
    $strictFailure = $false
    try { $null = Convert-MixedMemoryBytes ([byte[]](255, 123, 10)) } catch { $strictFailure = $true }
    if (-not $strictFailure) { throw 'Invalid input was silently accepted.' }
    'PASS: mixed ASCII/UTF-16/UTF-8 records preserved exactly; invalid bytes rejected.'
    return
}

$projectRoot = Split-Path $PSScriptRoot -Parent
$backupRoot = Join-Path $projectRoot ('Saved/MemoryRecovery/' + [DateTime]::UtcNow.ToString('yyyyMMddTHHmmssfffZ'))
foreach ($agentId in $AgentIds) {
    if ($agentId -notmatch '^[A-Za-z0-9_-]+$') { throw 'Invalid agent id.' }
    $agentDir = Join-Path $projectRoot ('Agents/' + $agentId)
    $memoryPath = Join-Path $agentDir 'memory.jsonl'
    if (-not (Test-Path -LiteralPath $memoryPath)) { continue }
    $leaseDir = Join-Path $agentDir '.gateway'
    $null = New-Item -ItemType Directory -Path $leaseDir -Force
    # The gateway holds this same exclusive lease throughout a headless turn.
    # Operator must stop PIE first; do not migrate live Unreal memory caches.
    $lease = [IO.File]::Open((Join-Path $leaseDir 'runtime.lock'), [IO.FileMode]::OpenOrCreate, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None)
    try {
        $original = [IO.File]::ReadAllBytes($memoryPath)
        $lines = Convert-MixedMemoryBytes $original
        $normalized = [Text.UTF8Encoding]::new($false, $true).GetBytes(($lines -join "`n") + "`n")
        $roundtrip = Convert-MixedMemoryBytes $normalized
        if ($lines.Count -ne $roundtrip.Count -or ($lines -join "`n") -cne ($roundtrip -join "`n")) { throw 'Round-trip validation failed.' }
        $oldHash = [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($original))
        # Both LF and CRLF are valid JSONL. Do not rewrite a healthy UTF-8 file
        # merely because Unreal and the gateway use different line endings.
        $needsRepair = $true
        try {
            $originalText = [Text.UTF8Encoding]::new($false, $true).GetString($original)
            $utf8Lines = @($originalText -split "`n" | ForEach-Object { $_.Trim() } | Where-Object { $_.Length -gt 0 })
            $needsRepair = ($utf8Lines.Count -ne $lines.Count -or ($utf8Lines -join "`n") -cne ($lines -join "`n"))
        } catch { $needsRepair = $true }
        if ($Repair -and $needsRepair) {
            $null = New-Item -ItemType Directory -Path $backupRoot -Force
            $tempPath = Join-Path $backupRoot ($agentId + '.utf8.tmp')
            $backupPath = Join-Path $backupRoot ($agentId + '.original.bin')
            [IO.File]::WriteAllBytes($tempPath, $normalized)
            $currentHash = [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([IO.File]::ReadAllBytes($memoryPath)))
            if ($currentHash -ne $oldHash) { throw 'Memory changed during preflight; replacement aborted.' }
            [IO.File]::Replace($tempPath, $memoryPath, $backupPath)
            if ([Convert]::ToHexString([Security.Cryptography.SHA256]::HashData([IO.File]::ReadAllBytes($backupPath))) -ne $oldHash) { throw 'Backup verification failed.' }
            "$agentId repaired: $($lines.Count) records; original backup: $backupPath"
        } else { "$agentId preflight: $($lines.Count) valid records; encoding repair needed: $needsRepair" }
    } finally { $lease.Dispose() }
}
