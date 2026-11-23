<#
.SYNOPSIS
Copies a stable Island data snapshot and builds its offline World Atlas.
.DESCRIPTION
Refuses to run while Unreal Editor or the packaged project game is open. Copies the selected data
root's WorldState/<Map>.json, optional <Map>.trails.json, and optional chronicle.jsonl into a new
timestamped directory, verifies SHA-256 stability, writes a provenance manifest, then runs the Atlas
builder on the copied files. The source root defaults to the project directory; pass -DataRoot when
the game used -CaptiveSkyDataRoot.
.EXAMPLE
./Scripts/Build-WorldAtlasSnapshot.ps1
./Scripts/Build-WorldAtlasSnapshot.ps1 -DataRoot Saved/Playtests/IslandSample
#>
param(
    [ValidatePattern('^[A-Za-z0-9_-]+$')]
    [string]$MapName = 'Island',
    [string]$DataRoot = (Join-Path $PSScriptRoot '..'),
    [string]$SnapshotsRoot = (Join-Path $PSScriptRoot '..\Saved\WorldAtlas\Snapshots')
)

$ErrorActionPreference = 'Stop'

$activeUnrealProcesses = @(Get-Process -ErrorAction SilentlyContinue | Where-Object {
    $_.ProcessName -match '^(UnrealEditor(-Cmd)?|CaptiveSky_2(-Win64-Shipping)?)$'
})
if ($activeUnrealProcesses.Count -gt 0) {
    $names = ($activeUnrealProcesses | ForEach-Object { "$($_.ProcessName) (PID $($_.Id))" }) -join ', '
    throw "Close Unreal Editor and the project game before snapshotting; still running: $names"
}

$projectRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$resolvedDataRoot = if ([System.IO.Path]::IsPathRooted($DataRoot)) {
    [System.IO.Path]::GetFullPath($DataRoot)
} else {
    [System.IO.Path]::GetFullPath((Join-Path $projectRoot $DataRoot))
}
$resolvedSnapshotsRoot = if ([System.IO.Path]::IsPathRooted($SnapshotsRoot)) {
    [System.IO.Path]::GetFullPath($SnapshotsRoot)
} else {
    [System.IO.Path]::GetFullPath((Join-Path $projectRoot $SnapshotsRoot))
}

$sourceDirectory = Join-Path $resolvedDataRoot 'WorldState'
$sourceFiles = @(
    [pscustomobject]@{ Name = "$MapName.json"; Path = (Join-Path $sourceDirectory "$MapName.json"); Required = $true },
    [pscustomobject]@{ Name = "$MapName.trails.json"; Path = (Join-Path $sourceDirectory "$MapName.trails.json"); Required = $false },
    [pscustomobject]@{ Name = 'chronicle.jsonl'; Path = (Join-Path $sourceDirectory 'chronicle.jsonl'); Required = $false }
)
$availableFiles = @()
foreach ($sourceFile in $sourceFiles) {
    if (Test-Path -LiteralPath $sourceFile.Path -PathType Leaf) {
        $availableFiles += $sourceFile
    } elseif ($sourceFile.Required) {
        throw "Required world-state snapshot is missing: $($sourceFile.Path)"
    }
}

$python = Get-Command python -ErrorAction SilentlyContinue
if (-not $python) { throw 'Python was not found on PATH.' }
$builderPath = Join-Path $PSScriptRoot 'Build-WorldAtlas.py'
if (-not (Test-Path -LiteralPath $builderPath -PathType Leaf)) {
    throw "World Atlas builder is missing: $builderPath"
}

$capturedAt = [DateTimeOffset]::UtcNow.ToString('O')
$snapshotName = "$MapName-$(Get-Date -Format 'yyyyMMdd-HHmmss-fff')"
$snapshotDirectory = Join-Path $resolvedSnapshotsRoot $snapshotName
if (Test-Path -LiteralPath $snapshotDirectory) {
    throw "Refusing to overwrite existing snapshot: $snapshotDirectory"
}
[System.IO.Directory]::CreateDirectory($snapshotDirectory) | Out-Null

$manifestFiles = @()
foreach ($sourceFile in $availableFiles) {
    $destinationPath = Join-Path $snapshotDirectory $sourceFile.Name
    $sourceHashBefore = (Get-FileHash -LiteralPath $sourceFile.Path -Algorithm SHA256).Hash
    Copy-Item -LiteralPath $sourceFile.Path -Destination $destinationPath
    $sourceHashAfter = (Get-FileHash -LiteralPath $sourceFile.Path -Algorithm SHA256).Hash
    $copiedHash = (Get-FileHash -LiteralPath $destinationPath -Algorithm SHA256).Hash
    if ($sourceHashBefore -ne $sourceHashAfter -or $sourceHashBefore -ne $copiedHash) {
        throw "Snapshot source changed during copy or failed verification: $($sourceFile.Path)"
    }
    $manifestFiles += [pscustomobject]@{
        name = $sourceFile.Name
        sha256 = $copiedHash
    }
}

$manifest = [ordered]@{
    map = $MapName
    captured_at_utc = $capturedAt
    data_root = $resolvedDataRoot
    files = $manifestFiles
}
$manifestJson = $manifest | ConvertTo-Json -Depth 4
[System.IO.File]::WriteAllText((Join-Path $snapshotDirectory 'manifest.json'), $manifestJson,
    [System.Text.UTF8Encoding]::new($false))

$worldStatePath = Join-Path $snapshotDirectory "$MapName.json"
$atlasPath = Join-Path $snapshotDirectory "$MapName-Atlas.html"
$arguments = @($builderPath, '--world-state', $worldStatePath, '--captured-at', "Snapshot captured UTC $capturedAt", '--out', $atlasPath)
$trailPath = Join-Path $snapshotDirectory "$MapName.trails.json"
$chroniclePath = Join-Path $snapshotDirectory 'chronicle.jsonl'
if (Test-Path -LiteralPath $trailPath -PathType Leaf) { $arguments += @('--trails', $trailPath) }
if (Test-Path -LiteralPath $chroniclePath -PathType Leaf) { $arguments += @('--chronicle', $chroniclePath) }
& $python.Source @arguments
if ($LASTEXITCODE -ne 0) { throw "World Atlas builder failed with exit code $LASTEXITCODE" }

Write-Output "Atlas: $atlasPath"
Write-Output "Snapshot manifest: $(Join-Path $snapshotDirectory 'manifest.json')"
