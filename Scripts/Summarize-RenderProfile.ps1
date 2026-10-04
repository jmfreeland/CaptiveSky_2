<#
.SYNOPSIS
Summarizes a selected frame window of an Unreal CSV profile without rewriting it.
.DESCRIPTION
StartRow is a zero-based CSV data-frame index, not the global engine frame count.
Choose the window from capture/log evidence, not from which window passes a gate.
Handles repeated Unreal counter headings and excludes non-frame footer rows.
Does not aggregate GPU scopes into a claimed total or infer critical-path cost.
#>
param(
    [Parameter(Mandatory)][string]$Path,
    [ValidateRange(0,2147483647)][int]$StartRow = 0,
    [ValidateRange(1,1000000)][int]$Count = 50
)
$ErrorActionPreference = 'Stop'
$lines = @(Get-Content -LiteralPath $Path)
if ($lines.Count -lt 2) { throw 'CSV profile has no data.' }
$seen = @{}
$headers = @($lines[0].Split(',') | ForEach-Object {
    $name = $_
    if ($seen.ContainsKey($name)) { $seen[$name]++; "$name#$($seen[$name])" }
    else { $seen[$name] = 0; $name }
})
if ($headers -notcontains 'FrameTime') { throw 'Not an Unreal frame profile: FrameTime heading missing.' }
$rows = @($lines | Select-Object -Skip 1 | ConvertFrom-Csv -Header $headers | Where-Object {
    $_.FrameTime -match '^\d+(\.\d*)?([eE][+-]?\d+)?$'
})
if ($StartRow -ge $rows.Count -or $Count -gt $rows.Count - $StartRow) {
    throw "Requested window exceeds the $($rows.Count) recorded data frames."
}
$window = @($rows | Select-Object -Skip $StartRow -First $Count)
$statistics = foreach ($metric in $headers | Where-Object { $_ -match '^FrameTime$|^GPU/|^Exclusive/(GameThread|RenderThread)/|^PSO/' }) {
    $values = @($window | ForEach-Object {
        $value = 0.0
        if ([double]::TryParse($_.$metric, [System.Globalization.NumberStyles]::Float,
                [System.Globalization.CultureInfo]::InvariantCulture, [ref]$value) -and [double]::IsFinite($value)) { $value }
    } | Sort-Object)
    if (-not $values.Count) {
        [pscustomobject]@{Metric=$metric; Samples=0; P50=$null; P95=$null; Max=$null; Sum=$null}
        continue
    }
    [pscustomobject]@{
        Metric=$metric; Samples=$values.Count
        P50=$values[[math]::Ceiling($values.Count * .50) - 1]
        P95=$values[[math]::Ceiling($values.Count * .95) - 1]
        Max=$values[-1]; Sum=($values | Measure-Object -Sum).Sum
    }
}
[pscustomobject]@{
    Path=(Resolve-Path -LiteralPath $Path).Path; TotalFrames=$rows.Count
    StartRow=$StartRow; Count=$Count; Statistics=@($statistics)
}
