<#
.SYNOPSIS
Runs an isolated, provider-free movement probe from the Island Inn to a tagged landmark.

.DESCRIPTION
Launches the Island as a bounded game world, disables background agent thinking,
and asks the runtime Innkeeper controller to move to a tagged landmark (the inn door
by default). The run uses a separate data root and never contacts the model provider.
It exits when movement completes, fails, or reaches its short timeout.
#>
param(
	[string]$DataRoot = "Saved/Playtests/InnMovementProbe",
	[string]$TargetTag = "InnDoorLantern",
	[ValidateRange(1, 1800)][int]$MaxRealtimeSeconds = 120,
	[string]$EngineDir = "D:\Games\Epic\UE_5.8"
)

$ErrorActionPreference = "Stop"
if ($TargetTag -notmatch '^[A-Za-z0-9_]+$') { throw "-TargetTag must be a single actor tag." }
$project = Resolve-Path (Join-Path $PSScriptRoot "..\CaptiveSky_2.uproject")
$editor = Join-Path $EngineDir "Engine\Binaries\Win64\UnrealEditor.exe"
if (-not (Test-Path $editor)) { throw "UnrealEditor.exe not found under $EngineDir; pass -EngineDir." }
$saved = Join-Path (Split-Path $project) "Saved"
$shaderWorkingDir = Join-Path $saved "ShaderWorking"
$logName = if ($TargetTag -eq "InnDoorLantern") { "InnMovementProbe.log" } else { "InnMovementProbe_$TargetTag.log" }
$log = Join-Path $saved "Logs\$logName"
New-Item -ItemType Directory -Force -Path $shaderWorkingDir | Out-Null

$arguments = @(
	$project, "/Game/Maps/Island", "-game", "-Spectator", "-windowed", "-ResX=1280", "-ResY=720",
	"-NullRHI", "-CaptiveSkyDisableAgentThinking", "-CaptiveSkyDataRoot=$DataRoot",
	"-CaptiveSkyMaxRealtimeSeconds=$MaxRealtimeSeconds", "-CaptiveSkyMaxModelRequests=1",
	"-ExecCmds=Island.MoveProbe IslandInnkeeper $TargetTag",
	"-abslog=$log", "-shaderworkingdir=$shaderWorkingDir", "-NoZen", "-DDC-ForceMemoryCache", "-nosound", "-unattended"
)
$game = Start-Process -FilePath $editor -ArgumentList ($arguments | ForEach-Object {
	if ("$_" -match '\s') { "`"$_`"" } else { "$_" }
}) -PassThru -Wait
if ($game.ExitCode -ne 0) { throw "Movement probe exited with code $($game.ExitCode); inspect $log" }
$results = Select-String -Path $log -Pattern "Isolated movement probe finished|Probe (completed|did not complete)|Timed out finding" |
	ForEach-Object { $_.Line }
$results
if (-not ($results -match "Isolated movement probe finished: success")) { throw "Innkeeper movement probe did not complete successfully; see $log" }
