if (-not ("CaptiveSky.Win32MemoryStatusEx" -as [type])) {
	Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;

namespace CaptiveSky {
	[StructLayout(LayoutKind.Sequential)]
	public sealed class Win32MemoryStatusEx {
		public uint Length;
		public uint MemoryLoad;
		public ulong TotalPhysical;
		public ulong AvailablePhysical;
		public ulong TotalPageFile;
		public ulong AvailablePageFile;
		public ulong TotalVirtual;
		public ulong AvailableVirtual;
		public ulong AvailableExtendedVirtual;

		public Win32MemoryStatusEx() {
			Length = (uint)Marshal.SizeOf(typeof(Win32MemoryStatusEx));
		}
	}

	public static class Win32Memory {
		[DllImport("kernel32.dll", SetLastError = true)]
		[return: MarshalAs(UnmanagedType.Bool)]
		public static extern bool GlobalMemoryStatusEx([In, Out] Win32MemoryStatusEx status);
	}
}
"@
}

function Get-AvailableSystemMemoryMiB {
	[CmdletBinding()]
	param()

	$status = [CaptiveSky.Win32MemoryStatusEx]::new()
	if (-not [CaptiveSky.Win32Memory]::GlobalMemoryStatusEx($status)) {
		$errorCode = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
		throw "GlobalMemoryStatusEx failed with Win32 error $errorCode."
	}
	[long][math]::Floor([double]$status.AvailablePhysical / 1MB)
}

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

Export-ModuleMember -Function Get-AvailableSystemMemoryMiB, Assert-SystemMemoryHeadroom, Get-SystemMemoryHeadroomMonitorDecision
