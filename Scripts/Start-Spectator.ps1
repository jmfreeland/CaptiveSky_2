<#
.SYNOPSIS
Opens the Island in spectator mode: a slow camera over the journey views that cuts to residents
when they speak or make something lasting.

.DESCRIPTION
Runs the project as a game (not the editor) with -Spectator. Residents think and remember as in any
play session. By default the usual bounded limits apply (Config/DefaultGame.ini: at most 30 real
minutes and 120 model requests), after which the game closes. -Continuous switches to continuous play
for an unattended screen: no end time, model requests drawn from a steadily refilling allowance, and a
daily ceiling (see AgentPlaySessionSubsystem). With -Continuous the world is relaunched if the game
crashes (not when it is closed normally), at most five times in any hour; each restart is noted in
Saved/Logs/SpectatorRestarts.log. Use -DataRoot and explicit caps for an isolated bounded sample; when
either cap is supplied, even -Continuous ends at that limit. Press ` and type Island.Spectate to toggle
back to normal control.

.EXAMPLE
./Scripts/Start-Spectator.ps1
./Scripts/Start-Spectator.ps1 -Windowed -Shots
./Scripts/Start-Spectator.ps1 -Windowed -Shots -ScreenshotDirectory Screenshots/Spectator/ReturnCheck
./Scripts/Start-Spectator.ps1 -Windowed -DataRoot Saved/Playtests/ReturnCheck -MaxRealtimeSeconds 600 -MaxModelRequests 10
./Scripts/Start-Spectator.ps1 -Windowed -Shots -DisableAgentThinking -DataRoot Saved/Playtests/TideglassMotion -MaxRealtimeSeconds 40 -MaxModelRequests 1 -ScreenshotDirectory Playtests/TideglassMotion/Screenshots -ViewpointFile Config/TideglassMotionProbe.json -EstablishingSeconds 10
./Scripts/Start-Spectator.ps1 -Windowed -Shots -DisableAgentThinking -DataRoot Saved/Playtests/GoldenHour -MaxRealtimeSeconds 120 -MaxModelRequests 1 -ViewpointFile Config/IslandViewpoints.json -ViewpointHour 17
./Scripts/Start-Spectator.ps1 -Windowed -Shots -DisableAgentThinking -DisableGroundCoverSway -NoZenLocalFallback -ForceMemoryDDC -DataRoot Saved/Playtests/FoliageSwayOff -MaxRealtimeSeconds 120 -MaxModelRequests 1 -ScreenshotDirectory Playtests/FoliageSwayOff/Screenshots -ViewpointFile Config/TideglassMotionProbe.json -CSVProfileFrames 200 -ShaderWorkingDir Saved/Playtests/FoliageSwayOff/ShaderWorking -LocalDataCachePath Saved/Playtests/FoliageSwayOff/DDC
./Scripts/Start-Spectator.ps1 -Windowed -Shots -DisableAgentThinking -FoliageSwayRadiusCm 180 -NoZenLocalFallback -ForceMemoryDDC -DataRoot Saved/Playtests/FoliageSway180 -MaxRealtimeSeconds 120 -MaxModelRequests 1 -ScreenshotDirectory Playtests/FoliageSway180/Screenshots -ViewpointFile Config/TideglassMotionProbe.json -CSVProfileFrames 200 -ShaderWorkingDir Saved/Playtests/FoliageSway180/ShaderWorking -LocalDataCachePath Saved/Playtests/FoliageSway180/DDC
./Scripts/Start-Spectator.ps1 -Windowed -Shots -DisableAgentThinking -DataRoot Saved/Playtests/WarmProfile -MaxRealtimeSeconds 180 -MaxModelRequests 1 -ScreenshotDirectory Playtests/WarmProfile/Screenshots -ViewpointFile Config/TideglassMotionProbe.json -CSVProfileFrames 200 -CSVProfileDelaySeconds 45
./Scripts/Start-Spectator.ps1 -Windowed -Shots -DisableAgentThinking -DataRoot Saved/Playtests/RenderTrace -MaxRealtimeSeconds 120 -MaxModelRequests 1 -ScreenshotDirectory Playtests/RenderTrace/Screenshots -ViewpointFile Config/TideglassMotionProbe.json -TraceProfileFile Saved/Profiling/Traces/RenderTrace.utrace -TraceProfileDelaySeconds 45 -TraceProfileDurationSeconds 30
./Scripts/Start-Spectator.ps1 -DisablePython -DisableAgentThinking -DataRoot Saved/Playtests/PythonOff -MaxRealtimeSeconds 60 -MaxModelRequests 1
./Scripts/Start-Spectator.ps1 -Continuous
#>
param(
	[switch]$Windowed,
	[switch]$Shots,
	[string]$ScreenshotDirectory,
	[string]$ViewpointFile,
	[string]$LogPath,
	[string]$ShaderWorkingDir,
	[string]$LocalDataCachePath,
	[string]$TraceProfileFile,
	[ValidateRange(1, 1800)][Nullable[int]]$TraceProfileDelaySeconds,
	[ValidateRange(5, 180)][Nullable[int]]$TraceProfileDurationSeconds,
	[ValidateRange(1, 600)][Nullable[int]]$EstablishingSeconds,
	[ValidateRange(0.0, 24.0)][Nullable[double]]$ViewpointHour,
	[ValidateRange(1, 2000)][Nullable[int]]$CSVProfileFrames,
	[ValidateRange(1, 1800)][Nullable[int]]$CSVProfileDelaySeconds,
	[ValidateRange(100, 3000)][Nullable[int]]$FoliageSwayRadiusCm,
	[switch]$DisableAgentThinking,
	[switch]$DisablePython,
	[switch]$DisableGroundCoverSway,
	[switch]$DisableDynamicGlobalIllumination,
	[switch]$NoZenLocalFallback,
	[switch]$ForceMemoryDDC,
	[switch]$Continuous,
	[string]$DataRoot,
	[ValidateRange(1, 1800)][Nullable[double]]$MaxRealtimeSeconds,
	[ValidateRange(1, 120)][Nullable[int]]$MaxModelRequests,
	[string]$EngineDir = "D:\Games\Epic\UE_5.8"
)

$ErrorActionPreference = "Stop"
$project = Resolve-Path (Join-Path $PSScriptRoot "..\CaptiveSky_2.uproject")
$projectRoot = Split-Path $project
$editor = Join-Path $EngineDir "Engine\Binaries\Win64\UnrealEditor.exe"
if (-not (Test-Path $editor)) { throw "UnrealEditor.exe not found under $EngineDir; pass -EngineDir." }

function Resolve-ProjectPath([string]$Path) {
	if ([string]::IsNullOrWhiteSpace($Path)) { return $Path }
	if ([System.IO.Path]::IsPathRooted($Path)) { return [System.IO.Path]::GetFullPath($Path) }
	return [System.IO.Path]::GetFullPath((Join-Path $projectRoot $Path))
}

$ScreenshotDirectory = Resolve-ProjectPath $ScreenshotDirectory
$DataRoot = Resolve-ProjectPath $DataRoot
$ShaderWorkingDir = Resolve-ProjectPath $ShaderWorkingDir
$LocalDataCachePath = Resolve-ProjectPath $LocalDataCachePath
$LogPath = Resolve-ProjectPath $LogPath
foreach ($outputDirectory in @($ScreenshotDirectory, $DataRoot, $ShaderWorkingDir, $LocalDataCachePath)) {
	if (-not [string]::IsNullOrWhiteSpace($outputDirectory)) {
		New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null
	}
}
if (-not [string]::IsNullOrWhiteSpace($TraceProfileFile)) {
	$TraceProfileFile = Resolve-ProjectPath $TraceProfileFile
	if (Test-Path -LiteralPath $TraceProfileFile) { throw "Trace output already exists; choose a new file: $TraceProfileFile" }
	$traceDirectory = Split-Path -Parent $TraceProfileFile
	if ($traceDirectory) { New-Item -ItemType Directory -Force -Path $traceDirectory | Out-Null }
	if (-not $PSBoundParameters.ContainsKey("TraceProfileDelaySeconds")) { $TraceProfileDelaySeconds = 45 }
	if (-not $PSBoundParameters.ContainsKey("TraceProfileDurationSeconds")) { $TraceProfileDurationSeconds = 30 }
} elseif ($PSBoundParameters.ContainsKey("TraceProfileDelaySeconds") -or $PSBoundParameters.ContainsKey("TraceProfileDurationSeconds")) {
	throw "-TraceProfileDelaySeconds and -TraceProfileDurationSeconds require -TraceProfileFile."
}

$arguments = @($project, "/Game/Maps/Island", "-game", "-Spectator")
$execCommands = @()
if ($Windowed) { $arguments += @("-windowed", "-ResX=1600", "-ResY=900") } else { $arguments += "-fullscreen" }
if ($Shots) { $arguments += "-SpectatorShots" } # one frame per shot; use -ScreenshotDirectory to isolate runs
if (-not [string]::IsNullOrWhiteSpace($ScreenshotDirectory)) { $arguments += "-SpectatorScreenshotDir=$ScreenshotDirectory" }
if (-not [string]::IsNullOrWhiteSpace($ViewpointFile)) {
	$viewpointPath = if ([System.IO.Path]::IsPathRooted($ViewpointFile)) { $ViewpointFile } else { Join-Path (Split-Path $project) $ViewpointFile }
	if (-not (Test-Path $viewpointPath)) { throw "Spectator viewpoint file not found: $viewpointPath" }
	$arguments += "-SpectatorViewpointFile=$viewpointPath"
}
if ($PSBoundParameters.ContainsKey("ViewpointHour")) {
	if (-not $Shots) { throw "-ViewpointHour requires -Shots; it is a capture-only clock override." }
	if ([string]::IsNullOrWhiteSpace($DataRoot)) { throw "-ViewpointHour requires an explicit isolated -DataRoot so the captured hour cannot overwrite your normal Island clock." }
	$hour = ([double]$ViewpointHour).ToString("0.###", [System.Globalization.CultureInfo]::InvariantCulture)
	$execCommands += "Island.Hour $hour"
}
if ($PSBoundParameters.ContainsKey("EstablishingSeconds")) { $arguments += "-SpectatorEstablishingSeconds=$EstablishingSeconds" }
if ($DisableAgentThinking) { $arguments += @("-CaptiveSkyDisableAgentThinking", "-unattended") }
if ($DisablePython) { $arguments += "-DisablePython" }
if ($DisableGroundCoverSway) { $arguments += "-IslandDisableGroundCoverSway" }
if ($NoZenLocalFallback) { $arguments += "-ddc=NoZenLocalFallback" }
if ($ForceMemoryDDC) { $arguments += "-DDC-ForceMemoryCache" }
if (-not [string]::IsNullOrWhiteSpace($ShaderWorkingDir)) { $arguments += "-shaderworkingdir=$ShaderWorkingDir" }
if (-not [string]::IsNullOrWhiteSpace($LocalDataCachePath)) { $arguments += "-LocalDataCachePath=$LocalDataCachePath" }
if (-not [string]::IsNullOrWhiteSpace($TraceProfileFile)) {
	$arguments += "-SpectatorTraceProfileFile=$TraceProfileFile"
	$arguments += "-SpectatorTraceProfileDelaySeconds=$TraceProfileDelaySeconds"
	$arguments += "-SpectatorTraceProfileDurationSeconds=$TraceProfileDurationSeconds"
}
if ($PSBoundParameters.ContainsKey("CSVProfileDelaySeconds")) {
	if (-not $PSBoundParameters.ContainsKey("CSVProfileFrames")) { throw "-CSVProfileDelaySeconds requires -CSVProfileFrames." }
	$arguments += "-SpectatorCSVProfileFrames=$CSVProfileFrames"
	$arguments += "-SpectatorCSVProfileDelaySeconds=$CSVProfileDelaySeconds"
} elseif ($PSBoundParameters.ContainsKey("CSVProfileFrames")) {
	$execCommands += "csvprofile frames=$CSVProfileFrames"
	$execCommands += "stat unit"
}
if ($PSBoundParameters.ContainsKey("FoliageSwayRadiusCm")) {
	$execCommands += "CaptiveSky.Island.FoliageSwayFocusRadiusCm $FoliageSwayRadiusCm"
}
if ($DisableDynamicGlobalIllumination) { $execCommands += "r.DynamicGlobalIlluminationMethod 0" }
if ($execCommands.Count -gt 0) { $arguments += "-ExecCmds=$($execCommands -join ',')" }
if ($Continuous) { $arguments += "-CaptiveSkyContinuous" }
if (-not [string]::IsNullOrWhiteSpace($DataRoot)) { $arguments += "-CaptiveSkyDataRoot=$DataRoot" }

$projectRoot = Split-Path $project
if (-not [string]::IsNullOrWhiteSpace($LogPath)) {
	New-Item -ItemType Directory -Force -Path (Split-Path $LogPath) | Out-Null
	$arguments += "-abslog=$LogPath"
}

if ($PSBoundParameters.ContainsKey("MaxRealtimeSeconds")) { $arguments += "-CaptiveSkyMaxRealtimeSeconds=$MaxRealtimeSeconds" }
if ($PSBoundParameters.ContainsKey("MaxModelRequests")) { $arguments += "-CaptiveSkyMaxModelRequests=$MaxModelRequests" }
if (-not $Continuous) { & $editor @arguments; return }
$arguments += "-unattended" # a crash must end the process rather than wait on a crash-report dialog

# An unattended screen should outlast an occasional engine crash (one was seen on a render worker thread
# after nine minutes). Residents' memories and the world state are saved as they go, so a relaunch resumes.
$restartLog = Join-Path $PSScriptRoot "..\Saved\Logs\SpectatorRestarts.log"
$recentCrashes = @()
while ($true) {
	$game = Start-Process -FilePath $editor -ArgumentList ($arguments | ForEach-Object { if ("$_" -match '\s') { "`"$_`"" } else { "$_" } }) -PassThru -Wait
	if ($game.ExitCode -eq 0) { break }
	$now = Get-Date
	$recentCrashes = @($recentCrashes | Where-Object { ($now - $_).TotalMinutes -lt 60 }) + $now
	New-Item -ItemType Directory -Force (Split-Path $restartLog) | Out-Null
	Add-Content -Path $restartLog -Encoding utf8 -Value "$($now.ToString('s')) game exited with code $($game.ExitCode); crash $($recentCrashes.Count) in the last hour."
	if ($recentCrashes.Count -ge 5) { Add-Content -Path $restartLog -Encoding utf8 -Value "Five crashes within an hour; not restarting."; break }
	Start-Sleep -Seconds 20
}
