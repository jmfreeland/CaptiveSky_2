<#
.SYNOPSIS
Runs an isolated, provider-free movement or wander probe for an Island resident.

.DESCRIPTION
Launches the Island as a bounded game world, disables background agent thinking,
and asks the selected resident controller to move to a tagged landmark (the inn door
and Innkeeper are the defaults), or to explicitly wander when `-TargetTag Wander`
is supplied. The run uses a separate data root and never contacts the model provider.
It exits when movement completes, fails, or reaches its short timeout.
#>
param(
	[string]$DataRoot = "Saved/Playtests/InnMovementProbe",
	[string]$MoverTag = "IslandInnkeeper",
	[string]$TargetTag = "InnDoorLantern",
	[switch]$CuriosityProbe,
	[string]$MoverStart = "",
	[ValidateRange(1, 1800)][int]$MaxRealtimeSeconds = 120,
	[string]$EngineDir = "D:\Games\Epic\UE_5.8"
)

$ErrorActionPreference = "Stop"
if ($TargetTag -notmatch '^[A-Za-z0-9_]+$') { throw "-TargetTag must be a single actor tag." }
if ($MoverTag -notmatch '^[A-Za-z0-9_]+$') { throw "-MoverTag must be a single actor tag or resolved agent id." }
if ($CuriosityProbe -and $TargetTag -ne "Wander") { throw "-CuriosityProbe requires -TargetTag Wander." }
if ($MoverStart -and $TargetTag -ne "Wander") { throw "-MoverStart is currently supported only for -TargetTag Wander." }
if ($MoverStart -and $MoverStart -notmatch '^-?\d+(\.\d+)?,-?\d+(\.\d+)?,-?\d+(\.\d+)?$') {
	throw "-MoverStart must be three comma-separated numeric world coordinates (x,y,z)."
}
$project = Resolve-Path (Join-Path $PSScriptRoot "..\CaptiveSky_2.uproject")
$editor = Join-Path $EngineDir "Engine\Binaries\Win64\UnrealEditor.exe"
if (-not (Test-Path $editor)) { throw "UnrealEditor.exe not found under $EngineDir; pass -EngineDir." }
$saved = Join-Path (Split-Path $project) "Saved"
$shaderWorkingDir = Join-Path $saved "ShaderWorking"
$probeSuffix = if ($CuriosityProbe) { "_Curiosity" } else { "" }
$logName = "MovementProbe_${MoverTag}_${TargetTag}${probeSuffix}.log"
$log = Join-Path $saved "Logs\$logName"
New-Item -ItemType Directory -Force -Path $shaderWorkingDir | Out-Null

$probeCommand = "Island.MoveProbe $MoverTag $TargetTag"
if ($CuriosityProbe) { $probeCommand += " Curious" }
if ($MoverStart) { $probeCommand += " " + $MoverStart.Replace(',', ' ') }

$arguments = @(
	$project, "/Game/Maps/Island", "-game", "-Spectator", "-windowed", "-ResX=1280", "-ResY=720",
	"-NullRHI", "-CaptiveSkyDisableAgentThinking", "-CaptiveSkyDataRoot=$DataRoot",
	"-CaptiveSkyMaxRealtimeSeconds=$MaxRealtimeSeconds", "-CaptiveSkyMaxModelRequests=1",
	"-ExecCmds=$probeCommand",
	"-abslog=$log", "-shaderworkingdir=$shaderWorkingDir", "-NoZen", "-DDC-ForceMemoryCache", "-nosound", "-unattended"
)
$game = Start-Process -FilePath $editor -ArgumentList ($arguments | ForEach-Object {
	if ("$_" -match '\s') { "`"$_`"" } else { "$_" }
}) -PassThru -Wait
if ($game.ExitCode -ne 0) { throw "Movement probe exited with code $($game.ExitCode); inspect $log" }
$results = Select-String -Path $log -Pattern "Isolated movement probe finished|Probe (completed|did not complete)|Timed out finding|curious flight-wander target|Forced curiosity probe" |
	ForEach-Object { $_.Line }
$results
if (-not ($results -match "Isolated movement probe finished: success")) { throw "Resident movement probe did not complete successfully; see $log" }
if ($CuriosityProbe -and -not ($results -match "chose a curious flight-wander target")) {
	throw "Raven flight completed, but the forced curiosity branch did not select a visible landmark; see $log"
}
