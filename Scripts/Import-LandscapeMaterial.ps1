param(
    [string]$SourceProject = 'D:\Projects - Athena\Unreal\CaptiveSky',
    [string]$RootAsset = '/Game/Materals/MI_MountainRange',
    [switch]$Copy
)
$ErrorActionPreference = 'Stop'
$sourceContent = [IO.Path]::GetFullPath((Join-Path $SourceProject 'Content'))
$targetContent = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../Content'))
$queue = [Collections.Generic.Queue[string]]::new()
$seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
$found = [Collections.Generic.List[object]]::new()
$queue.Enqueue($RootAsset)
while ($queue.Count -gt 0) {
    $package = $queue.Dequeue().Split('.')[0]
    if (!$seen.Add($package)) { continue }
    if (!$package.StartsWith('/Game/')) { throw "Unsupported mount: $package" }
    $relative = $package.Substring(6) + '.uasset'
    $source = [IO.Path]::GetFullPath((Join-Path $sourceContent $relative))
    $target = [IO.Path]::GetFullPath((Join-Path $targetContent $relative))
    if (!$source.StartsWith($sourceContent + [IO.Path]::DirectorySeparatorChar) -or !$target.StartsWith($targetContent + [IO.Path]::DirectorySeparatorChar)) { throw 'Asset path escapes Content' }
    if (!(Test-Path -LiteralPath $source)) {
        if (Test-Path -LiteralPath $source.Substring(0, $source.Length - 7) -PathType Container) { continue }
        throw "Missing dependency: $source"
    }
    $found.Add([pscustomobject]@{Package=$package; Source=$source; Target=$target; Bytes=(Get-Item -LiteralPath $source).Length})
    # Package strings are a conservative discovery aid; validate the loaded assets in Unreal afterwards.
    # Names/imports precede bulk texture payloads. Bound this preliminary scan;
    # Unreal's asset registry remains the authority for dependency validation.
    $stream = [IO.File]::OpenRead($source)
    try {
        $data = [byte[]]::new([Math]::Min($stream.Length, 1048576))
        [void]$stream.Read($data, 0, $data.Length)
    } finally { $stream.Dispose() }
    foreach ($encoding in @([Text.Encoding]::ASCII)) {
        foreach ($match in [regex]::Matches($encoding.GetString($data), '/Game/[A-Za-z0-9_/.-]+')) {
            $queue.Enqueue($match.Value)
        }
    }
}
# Preflight the entire closure before copying anything; never overwrite unrelated project assets.
foreach ($asset in $found) {
    if ((Test-Path -LiteralPath $asset.Target) -and (Get-FileHash -LiteralPath $asset.Source).Hash -ne (Get-FileHash -LiteralPath $asset.Target).Hash) {
        throw "Conflicting destination: $($asset.Target)"
    }
}
$found | Select-Object Package,Bytes
"Assets: $($found.Count); MiB: $([math]::Round(($found | Measure-Object Bytes -Sum).Sum / 1MB, 1))"
if ($Copy) {
    foreach ($asset in $found) {
        if (!(Test-Path -LiteralPath $asset.Target)) {
            New-Item -ItemType Directory -Path ([IO.Path]::GetDirectoryName($asset.Target)) -Force | Out-Null
            Copy-Item -LiteralPath $asset.Source -Destination $asset.Target
        }
    }
    'Copied dependency closure; source project unchanged.'
}
