function Assert-SystemMemoryHeadroom {
	[CmdletBinding()]
	param(
		[Parameter(Mandatory = $true)][long]$FreeMiB,
		[ValidateRange(512, 32768)][int]$MinimumFreeMiB = 4096
	)

	if ($FreeMiB -lt 0) { throw "Invalid system memory budget: free=$FreeMiB MiB." }
	if ($FreeMiB -lt $MinimumFreeMiB) {
		throw "System memory preflight failed: only $FreeMiB MiB free; at least $MinimumFreeMiB MiB is required. No rendered session was started."
	}

	[pscustomobject]@{ FreeMiB = $FreeMiB }
}

function Get-SystemMemoryHeadroomMonitorDecision {
	[CmdletBinding()]
	param(
		[Parameter(Mandatory = $true)][long]$FreeMiB,
		[ValidateRange(512, 32768)][int]$MinimumFreeMiB = 4096,
		[ValidateRange(0, 5)][int]$ConsecutiveLowSamples = 0,
		[ValidateRange(1, 5)][int]$RequiredConsecutiveLowSamples = 2
	)

	if ($FreeMiB -lt 0) { throw "Invalid system memory budget: free=$FreeMiB MiB." }
	$nextLowSamples = if ($FreeMiB -lt $MinimumFreeMiB) { $ConsecutiveLowSamples + 1 } else { 0 }
	[pscustomobject]@{
		FreeMiB = $FreeMiB
		ConsecutiveLowSamples = $nextLowSamples
		ShouldStop = ($nextLowSamples -ge $RequiredConsecutiveLowSamples)
	}
}

Export-ModuleMember -Function Assert-SystemMemoryHeadroom, Get-SystemMemoryHeadroomMonitorDecision
