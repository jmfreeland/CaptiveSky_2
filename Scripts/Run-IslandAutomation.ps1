param(
	[Parameter(Mandatory = $true)]
	[ValidatePattern('^[A-Za-z0-9_.+]+$')]
	[string]$Test,
	[string]$EngineDir = 'D:\Games\Epic\UE_5.8',
	[ValidateRange(30, 900)][int]$TimeoutSeconds = 240,
	[ValidateRange(512, 32768)][int]$MinimumFreeSystemMemoryMiB = 4096,
	[switch]$DryRun
)

$ErrorActionPreference = 'Stop'
$projectRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$project = Join-Path $projectRoot 'CaptiveSky_2.uproject'
$editor = Join-Path $EngineDir 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$playtestsRoot = [System.IO.Path]::GetFullPath((Join-Path $projectRoot 'Saved\Playtests'))
$runId = '{0}_{1}' -f (Get-Date -Format 'yyyyMMdd_HHmmss'), ([guid]::NewGuid().ToString('N').Substring(0, 8))
$dataRoot = Join-Path $playtestsRoot (Join-Path "Automation_$runId" 'Data')
$logDirectory = Join-Path $projectRoot 'Saved\Logs'
$logPath = Join-Path $logDirectory "Automation_$runId.log"

if (-not (Test-Path -LiteralPath $project -PathType Leaf)) { throw "Project file not found: $project" }
if (-not (Test-Path -LiteralPath $editor -PathType Leaf)) { throw "UnrealEditor-Cmd.exe not found: $editor" }
if (Test-Path -LiteralPath $dataRoot) { throw "Refusing to reuse an existing automation data root: $dataRoot" }

Import-Module (Join-Path $PSScriptRoot 'SystemMemoryBudget.psm1') -Force
$freeSystemMemoryMiB = Get-AvailableSystemMemoryMiB
Write-Output "Available system memory: $freeSystemMemoryMiB MiB (minimum $MinimumFreeSystemMemoryMiB MiB)"
if (-not $DryRun) {
	Assert-SystemMemoryHeadroom -FreeMiB $freeSystemMemoryMiB -MinimumFreeMiB $MinimumFreeSystemMemoryMiB | Out-Null
}

$arguments = @(
	'"' + $project + '"'
	'-unattended'
	'-nop4'
	'-nosplash'
	'-NullRHI'
	'-NoSound'
	'-DisablePython'
	'-NoZen'
	'-notraceserver'
	'-DDC-ForceMemoryCache'
	'-CaptiveSkyDisableAgentThinking'
	'-CaptiveSkyMaxModelRequests=0'
	'-CaptiveSkyMaxRealtimeSeconds=90'
	('"-CaptiveSkyDataRoot=' + $dataRoot + '"')
	('"-abslog=' + $logPath + '"')
	('"-ExecCmds=Automation RunTests ' + $Test + '; Quit"')
	'"-TestExit=Automation Test Queue Empty"'
)
$argumentLine = [string]::Join(' ', $arguments)

Write-Output "Project: $project"
Write-Output "Automation: $Test"
Write-Output "Isolated data: $dataRoot"
Write-Output "Log: $logPath"
Write-Output "Timeout: $TimeoutSeconds seconds"
if ($DryRun) {
	Write-Output "Executable: $editor"
	Write-Output "Arguments: $argumentLine"
	exit 0
}

[System.IO.Directory]::CreateDirectory($logDirectory) | Out-Null
$launchTime = Get-Date
$process = Start-Process -FilePath $editor -ArgumentList $argumentLine -WorkingDirectory (Split-Path -Parent $editor) -WindowStyle Hidden -PassThru
$timer = [System.Diagnostics.Stopwatch]::StartNew()
$consecutiveLowSamples = 0
try {
	while (-not $process.WaitForExit(5000)) {
		$freeSystemMemoryMiB = Get-AvailableSystemMemoryMiB
		$decision = Get-SystemMemoryHeadroomMonitorDecision -FreeMiB $freeSystemMemoryMiB `
			-MinimumFreeMiB $MinimumFreeSystemMemoryMiB -ConsecutiveLowSamples $consecutiveLowSamples
		$consecutiveLowSamples = $decision.ConsecutiveLowSamples
		Write-Output ("RAM: {0} MiB free, low samples {1}/2; automation elapsed {2}s." -f
			$freeSystemMemoryMiB, $consecutiveLowSamples, [math]::Floor($timer.Elapsed.TotalSeconds))
		if ($decision.ShouldStop) {
			$process.Kill($true)
			$process.WaitForExit()
			throw "Stopped only the Editor-Cmd process tree started by this script after system RAM remained below $MinimumFreeSystemMemoryMiB MiB. Log: $logPath"
		}
		if ($timer.Elapsed.TotalSeconds -ge $TimeoutSeconds) {
			$process.Kill($true)
			$process.WaitForExit()
			throw "Automation timed out after $TimeoutSeconds seconds; stopped only the Editor-Cmd process tree started by this script. Log: $logPath"
		}
	}
	$exitCode = $process.ExitCode
}
finally {
	$process.Dispose()
	$timer.Stop()
}

if (Test-Path -LiteralPath $logPath -PathType Leaf) {
	Get-Content -LiteralPath $logPath -Tail 45
}
if (-not (Test-Path -LiteralPath $logPath -PathType Leaf)) {
	$crashRoot = Join-Path $projectRoot 'Saved\Crashes'
	$crashContext = Get-ChildItem -LiteralPath $crashRoot -Filter 'CrashContext.runtime-xml' -File -Recurse -ErrorAction SilentlyContinue |
		Where-Object { $_.LastWriteTime -ge $launchTime } |
		Sort-Object LastWriteTime -Descending |
		Select-Object -First 1
	if ($crashContext) {
		$contextText = Get-Content -LiteralPath $crashContext.FullName -Raw
		$errorMatch = [regex]::Match($contextText, '<ErrorMessage>(.*?)</ErrorMessage>', [System.Text.RegularExpressions.RegexOptions]::Singleline)
		$errorMessage = if ($errorMatch.Success) { [System.Net.WebUtility]::HtmlDecode($errorMatch.Groups[1].Value.Trim()) } else { 'No crash message recorded.' }
		throw "UnrealEditor-Cmd exited with code 0 but wrote no requested log. Crash: $errorMessage Crash report: $($crashContext.FullName)"
	}
	throw "UnrealEditor-Cmd produced no log: $logPath"
}
if ($exitCode -ne 0) { throw "UnrealEditor-Cmd exited with code $exitCode. Log: $logPath" }
$logText = Get-Content -LiteralPath $logPath -Raw
foreach ($testPath in ($Test -split '\+')) {
	$escapedTestPath = [regex]::Escape($testPath)
	if ($logText -notmatch "Test Completed\. Result=\{Success\} Name=\{[^}]+\} Path=\{$escapedTestPath\}") {
		throw "The requested automation did not report a successful completion: $testPath. Log: $logPath"
	}
}
Write-Output 'Every requested automation reported Result={Success}.'
