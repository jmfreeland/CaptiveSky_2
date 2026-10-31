function ConvertFrom-NvidiaGpuMemoryOutput {
	[CmdletBinding()]
	param(
		[Parameter(Mandatory = $true)]
		[string[]]$OutputLines
	)

	$rows = @($OutputLines | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
	if ($rows.Count -lt 1) { throw "nvidia-smi returned no GPU memory row." }
	if ($rows[0].Trim() -notmatch '^\s*(?<used>\d+)\s*,\s*(?<total>\d+)\s*$') {
		throw "nvidia-smi returned an invalid GPU memory row: $($rows[0])"
	}

	$usedMiB = [int]$Matches.used
	$totalMiB = [int]$Matches.total
	if ($totalMiB -le 0 -or $usedMiB -lt 0 -or $usedMiB -gt $totalMiB) {
		throw "nvidia-smi returned out-of-range GPU memory values: used=$usedMiB MiB, total=$totalMiB MiB."
	}

	[pscustomobject]@{
		UsedMiB  = $usedMiB
		TotalMiB = $totalMiB
	}
}

function Assert-GpuMemoryHeadroom {
	[CmdletBinding()]
	param(
		[Parameter(Mandatory = $true)][int]$UsedMiB,
		[Parameter(Mandatory = $true)][int]$TotalMiB,
		[ValidateRange(512, 16384)][int]$MinimumFreeMiB = 4096
	)

	if ($TotalMiB -le 0 -or $UsedMiB -lt 0 -or $UsedMiB -gt $TotalMiB) {
		throw "Invalid GPU memory budget: used=$UsedMiB MiB, total=$TotalMiB MiB."
	}
	$freeMiB = $TotalMiB - $UsedMiB
	if ($freeMiB -lt $MinimumFreeMiB) {
		throw "GPU memory preflight failed: only $freeMiB MiB free; at least $MinimumFreeMiB MiB is required. No rendered session was started."
	}

	[pscustomobject]@{
		FreeMiB  = $freeMiB
		TotalMiB = $TotalMiB
	}
}

Export-ModuleMember -Function ConvertFrom-NvidiaGpuMemoryOutput, Assert-GpuMemoryHeadroom
