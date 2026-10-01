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
./Scripts/Start-Spectator.ps1 -Continuous
#>
param(
	[switch]$Windowed,
	[switch]$Shots,
	[string]$ScreenshotDirectory,
	[string]$ViewpointFile,
	[ValidateRange(1, 600)][Nullable[int]]$EstablishingSeconds,
	[ValidateRange(0.0, 24.0)][Nullable[double]]$ViewpointHour,
	[switch]$DisableAgentThinking,
	[switch]$Continuous,
	[string]$DataRoot,
	[ValidateRange(1, 1800)][Nullable[double]]$MaxRealtimeSeconds,
	[ValidateRange(1, 120)][Nullable[int]]$MaxModelRequests,
	[string]$EngineDir = "D:\Games\Epic\UE_5.8"
)

$ErrorActionPreference = "Stop"
$project = Resolve-Path (Join-Path $PSScriptRoot "..\CaptiveSky_2.uproject")
$editor = Join-Path $EngineDir "Engine\Binaries\Win64\UnrealEditor.exe"
if (-not (Test-Path $editor)) { throw "UnrealEditor.exe not found under $EngineDir; pass -EngineDir." }

$arguments = @($project, "/Game/Maps/Island", "-game", "-Spectator")
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
	$arguments += "-ExecCmds=Island.Hour $hour"
}
if ($PSBoundParameters.ContainsKey("EstablishingSeconds")) { $arguments += "-SpectatorEstablishingSeconds=$EstablishingSeconds" }
if ($DisableAgentThinking) { $arguments += @("-CaptiveSkyDisableAgentThinking", "-unattended") }
if ($Continuous) { $arguments += "-CaptiveSkyContinuous" }
if (-not [string]::IsNullOrWhiteSpace($DataRoot)) { $arguments += "-CaptiveSkyDataRoot=$DataRoot" }
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
