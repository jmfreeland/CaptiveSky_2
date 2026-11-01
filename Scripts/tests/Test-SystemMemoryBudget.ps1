$ErrorActionPreference = "Stop"
Import-Module (Join-Path $PSScriptRoot "..\SystemMemoryBudget.psm1") -Force

$budget = Assert-SystemMemoryHeadroom -FreeMiB 4096 -MinimumFreeMiB 4096
if ($budget.FreeMiB -ne 4096) { throw "Exact system-memory reserve boundary should pass." }

$failedClosed = $false
try {
	Assert-SystemMemoryHeadroom -FreeMiB 4095 -MinimumFreeMiB 4096 | Out-Null
} catch {
	$failedClosed = $_.Exception.Message.Contains("No rendered session was started")
}
if (-not $failedClosed) { throw "An over-budget system-memory snapshot must fail closed." }

$invalidFailed = $false
try {
	Assert-SystemMemoryHeadroom -FreeMiB -1 | Out-Null
} catch {
	$invalidFailed = $true
}
if (-not $invalidFailed) { throw "Negative free system memory must be rejected." }

$firstLowSample = Get-SystemMemoryHeadroomMonitorDecision -FreeMiB 3000 -MinimumFreeMiB 4096
if ($firstLowSample.ConsecutiveLowSamples -ne 1 -or $firstLowSample.ShouldStop) {
	throw "One low system-memory sample should warn but not stop a rendered session."
}
$secondLowSample = Get-SystemMemoryHeadroomMonitorDecision -FreeMiB 2999 -MinimumFreeMiB 4096 -ConsecutiveLowSamples $firstLowSample.ConsecutiveLowSamples
if ($secondLowSample.ConsecutiveLowSamples -ne 2 -or -not $secondLowSample.ShouldStop) {
	throw "Two consecutive low system-memory samples should trigger a guarded stop."
}
$recoveredSample = Get-SystemMemoryHeadroomMonitorDecision -FreeMiB 4096 -MinimumFreeMiB 4096 -ConsecutiveLowSamples $firstLowSample.ConsecutiveLowSamples
if ($recoveredSample.ConsecutiveLowSamples -ne 0 -or $recoveredSample.ShouldStop) {
	throw "A recovered system-memory sample should reset the low-sample streak."
}

Write-Output "System memory budget tests passed."
