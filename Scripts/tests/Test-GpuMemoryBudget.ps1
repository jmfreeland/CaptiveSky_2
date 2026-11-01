$ErrorActionPreference = "Stop"
Import-Module (Join-Path $PSScriptRoot "..\GpuMemoryBudget.psm1") -Force

$snapshot = ConvertFrom-NvidiaGpuMemoryOutput -OutputLines @("9888, 12282")
if ($snapshot.UsedMiB -ne 9888 -or $snapshot.TotalMiB -ne 12282) {
	throw "NVIDIA memory output was not parsed correctly."
}

$budget = Assert-GpuMemoryHeadroom -UsedMiB 8192 -TotalMiB 12288 -MinimumFreeMiB 4096
if ($budget.FreeMiB -ne 4096) { throw "Exact reserve boundary should pass." }

$failedClosed = $false
try {
	Assert-GpuMemoryHeadroom -UsedMiB 8193 -TotalMiB 12288 -MinimumFreeMiB 4096 | Out-Null
} catch {
	$failedClosed = $_.Exception.Message.Contains("No rendered session was started")
}
if (-not $failedClosed) { throw "An over-budget GPU snapshot must fail closed." }

$malformedFailed = $false
try {
	ConvertFrom-NvidiaGpuMemoryOutput -OutputLines @("N/A, 12282") | Out-Null
} catch {
	$malformedFailed = $true
}
if (-not $malformedFailed) { throw "Malformed nvidia-smi output must fail closed." }

$firstLowSample = Get-GpuHeadroomMonitorDecision -UsedMiB 9000 -TotalMiB 12288 -MinimumFreeMiB 4096
if ($firstLowSample.FreeMiB -ne 3288 -or $firstLowSample.ConsecutiveLowSamples -ne 1 -or $firstLowSample.ShouldStop) {
	throw "One low sample should warn but not stop a rendered session."
}
$secondLowSample = Get-GpuHeadroomMonitorDecision -UsedMiB 9001 -TotalMiB 12288 -MinimumFreeMiB 4096 -ConsecutiveLowSamples $firstLowSample.ConsecutiveLowSamples
if ($secondLowSample.ConsecutiveLowSamples -ne 2 -or -not $secondLowSample.ShouldStop) {
	throw "Two consecutive low samples should trigger a guarded stop."
}
$recoveredSample = Get-GpuHeadroomMonitorDecision -UsedMiB 8192 -TotalMiB 12288 -MinimumFreeMiB 4096 -ConsecutiveLowSamples $firstLowSample.ConsecutiveLowSamples
if ($recoveredSample.ConsecutiveLowSamples -ne 0 -or $recoveredSample.ShouldStop) {
	throw "A recovered headroom sample should reset the low-sample streak."
}

Write-Output "GPU memory budget tests passed."
